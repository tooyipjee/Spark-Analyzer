#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include "tcpm_driver.h"
#include "usb_pd.h"
#include <Preferences.h>
#include <Adafruit_NeoPixel.h> // Include NeoPixel Library
#define PIN 8
#define NUMPIXELS 175

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// Filter variables
#define MOVING_AVERAGE_LENGTH 10
int adcSamples[MOVING_AVERAGE_LENGTH];
int adcIndex = 0;
int adcSum = 0;

void sendDataPacket();
int readFilteredADC(int pin);
void updateUCPDVoltage(int newVoltage);

bool deviceConnected = false;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 500; // 500ms
Preferences preferences;
int setVoltage;
const int usb_pd_int_pin = 10;
const int debug_led_pin  = 3;
const int current_pin = 2;
int current = 0;
bool output = 0;         
int voltage = 5;        
int currentLimit = 0; 
int filterBuf = 0;
int adcError = 0;
int loopCount = 0;
int sampledVal = 0; 

// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};
BLECharacteristic *pCharacteristic;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device Connected");
      sendDataPacket();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device Disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.println("Received data:");
        for (int i = 0; i < value.length(); i++) {
          Serial.print(value[i]);
        }
        Serial.println();

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, value.c_str());

        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }

        output = doc["output"];
        voltage = doc["voltage"];
        currentLimit = doc["currentLimit"];

        Serial.print("Output: "); Serial.println(output);
        Serial.print("Voltage: "); Serial.println(voltage);
        Serial.print("Current Limit: "); Serial.println(currentLimit);

        if (voltage != setVoltage) {
          Serial.println("Updating voltage setting...");
          setVoltage = voltage;
          preferences.putInt("Voltage", voltage);
          updateUCPDVoltage(voltage); 
          ESP.restart();

        }
      }
    }
};

void setup() {
  for (int i = 0; i < MOVING_AVERAGE_LENGTH; i++) {
    adcSamples[i] = analogRead(current_pin);
    adcSum += adcSamples[i];
    delay(1); 
  }
  pixels.begin(); // Initialize NeoPixel strip

  Serial.begin(115200);
  Serial.println("Starting BLE...");

  BLEDevice::init("Spark Analyzer");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID,
                    BLECharacteristic::PROPERTY_READ |
                    BLECharacteristic::PROPERTY_WRITE |
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Service and Characteristic created, advertising started.");

  preferences.begin("credentials", false); 
  setVoltage = preferences.getInt("Voltage");
  
  pinMode(usb_pd_int_pin, INPUT);
  pinMode(debug_led_pin, OUTPUT);
  digitalWrite(debug_led_pin, HIGH);
  pinMode(current_pin, INPUT);

  Wire.begin(1,0);
  Wire.setClock(400000);
  delay(500);
  digitalWrite(debug_led_pin, LOW);
  tcpm_init(0);
  delay(50);
  pd_init(0);
  delay(50);

  updateUCPDVoltage(setVoltage);
  delay(50);
}

void loop() {
  fadeColors();

  if (millis() - lastUpdateTime >= updateInterval) {
    sendDataPacket();
    lastUpdateTime = millis();
  }

  if(output == 1) {
    digitalWrite(debug_led_pin, HIGH);
  } else {
    adcError = readFilteredADC(current_pin);
    digitalWrite(debug_led_pin, LOW);
  }
  current = 5.6865*(readFilteredADC(current_pin)-adcError);

  if(current > currentLimit && currentLimit != 0) {
    output = 0;
  }
  if (LOW == digitalRead(usb_pd_int_pin)) {
    tcpc_alert(0);
  }
  pd_run_state_machine(0);
  delay(4);
}

void sendDataPacket() {
  if (deviceConnected) {
    DynamicJsonDocument doc(1024);

    doc["Voltage"] = setVoltage;
    doc["Current"] = current;
    doc["OutputEN"] = output;
    doc["currentLimit"] = currentLimit;
    std::string jsonData;
    serializeJson(doc, jsonData);
    pCharacteristic->setValue(jsonData);
    pCharacteristic->notify(true);

    Serial.println("Data Packet Sent:");
    serializeJsonPretty(doc, Serial);
    Serial.println();
  }
}

void updateUCPDVoltage(int newVoltage) {
  
  if(newVoltage == 5) {
    pd_set_max_voltage(5000);  
  } else if(newVoltage == 9) {
    pd_set_max_voltage(9000);  
  } else if(newVoltage == 12) {
    pd_set_max_voltage(12000);  
  } else if(newVoltage == 15) {
    pd_set_max_voltage(15000);  
  } else if(newVoltage == 20) {
    pd_set_max_voltage(20000);  
  }
  tcpc_alert(0);
}

int readFilteredADC(int pin) {
  int newSample = analogRead(pin);
  adcSum -= adcSamples[adcIndex];
  adcSamples[adcIndex] = newSample;
  adcSum += newSample;
  adcIndex = (adcIndex + 1) % MOVING_AVERAGE_LENGTH;
  return adcSum / MOVING_AVERAGE_LENGTH;
}
void fadeColors() {
    static unsigned long lastUpdate = 0;
    const long interval = 50; // Time interval for color changes
    static int fadeValue = 0;
    static int stage = 0;

    unsigned long currentMillis = millis();

    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;

        switch (stage) {
            case 0: // Fade in blue
                setStripColor(0, 0, fadeValue);
                break;
            case 1: // Transition to green
                setStripColor(0, fadeValue, 255 - fadeValue);
                break;
            case 2: // Transition to red
                setStripColor(fadeValue, 255 - fadeValue, 0);
                break;
            case 3: // Transition to blue (from red)
                setStripColor(255 - fadeValue, 0, fadeValue);
                break;
            case 4: // Fade out to off
                setStripColor(0, 0, 255 - fadeValue);
                break;
        }

        pixels.show();
        fadeValue++;

        if (fadeValue > 255) {
            fadeValue = 0;
            stage++;
            if (stage > 4) stage = 0; // Reset to initial stage after completion
        }
    }
}

void setStripColor(uint8_t red, uint8_t green, uint8_t blue) {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(red, green, blue));
    }
}
