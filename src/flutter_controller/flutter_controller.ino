#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
void sendDataPacket();
bool deviceConnected = false;
BLECharacteristic *pCharacteristic;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 500; // 500ms
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

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
        deserializeJson(doc, value.c_str());
        serializeJsonPretty(doc, Serial);
        Serial.println();
        
        // // Send data packet after processing received data
        // sendDataPacket();
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
}


void loop() {
  // Check if it's time to send an update
  if (millis() - lastUpdateTime >= updateInterval) {
    sendDataPacket();
    lastUpdateTime = millis();
  }
}

void sendDataPacket() {
  if (deviceConnected) {
    DynamicJsonDocument doc(1024);

    doc["Voltage"] = random(1,20); // You can replace this with actual voltage data
    doc["Current"] = random(0,2500); // You can replace this with actual current data
    doc["OutputEN"] = true; // You can replace this with actual OutputEN status

    std::string jsonData;
    serializeJson(doc, jsonData);
    pCharacteristic->setValue(jsonData);
    pCharacteristic->notify(true);  // Pass true to indicate confirmation is required

    Serial.println("Data Packet Sent:");
    serializeJsonPretty(doc, Serial);
    Serial.println();
  }
}
