#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include "tcpm_driver.h"
#include "usb_pd.h"
#include <Preferences.h>

#define FILTER_LENGTH 1000
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void sendDataPacket();

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
      
      // Send data packet when device is connected
      sendDataPacket();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device Disconnected");
      ESP.restart();
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

        // Assuming the received data is a JSON string
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, value.c_str());

        // Check if parsing was successful
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }

        // Extracting the values from the JSON document
        output = doc["output"];         // Extracting boolean value for "output"
        voltage = doc["voltage"];        // Extracting integer value for "voltage"
        currentLimit = doc["currentLimit"]; // Extracting integer value for "currentLimit"

        // You can now use the variables output, voltage, and currentLimit in your code
        Serial.print("Output: "); Serial.println(output);
        Serial.print("Voltage: "); Serial.println(voltage);
        Serial.print("Current Limit: "); Serial.println(currentLimit);


        if (voltage != setVoltage)
        {
          Serial.println("Voltage changed!");
          preferences.putInt("Voltage", voltage);
          preferences.end();
          ESP.restart();
        }
      }
    }
};
void setup() {
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

  if(setVoltage == 5)
  {
    pd_set_max_voltage(5000);  
  }
  else if(setVoltage == 9)
  {
    pd_set_max_voltage(9000);  
  }
  else if(setVoltage == 15)
  {
    pd_set_max_voltage(15000);  
  }
  else if(setVoltage == 20)
  {
    pd_set_max_voltage(20000);  
  }
}


void loop() {
  // Check if it's time to send an update
  if (millis() - lastUpdateTime >= updateInterval) {
    sendDataPacket();
    lastUpdateTime = millis();
  }

  pd_run_state_machine(0);
    if(output == 1)
  {
    digitalWrite(debug_led_pin, HIGH);
  }
  else
  {
    digitalWrite(debug_led_pin,LOW);
  }

}

void sendDataPacket() {
  if (deviceConnected) {
    DynamicJsonDocument doc(1024);

    doc["Voltage"] = setVoltage; // You can replace this with actual voltage data
    doc["Current"] = analogRead(current_pin); // You can replace this with actual current data
    doc["OutputEN"] = output; // You can replace this with actual OutputEN status

    std::string jsonData;
    serializeJson(doc, jsonData);
    pCharacteristic->setValue(jsonData);
    pCharacteristic->notify(true);  // Pass true to indicate confirmation is required

    Serial.println("Data Packet Sent:");
    serializeJsonPretty(doc, Serial);
    Serial.println();
  }
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  digitalWrite(debug_led_pin, HIGH);

  Serial.print("Voltage set to ");
  Serial.println(PD_MAX_VOLTAGE_MV);
}