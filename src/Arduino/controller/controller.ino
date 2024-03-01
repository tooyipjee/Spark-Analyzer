/*
   -- Spark Analyzer BLE Communication Firmware --

   This firmware enables Bluetooth Low Energy (BLE) communication for the Spark Analyzer,
   facilitating data exchange between the device and a BLE client (e.g., a smartphone app).
   The firmware uses the Arduino BLE library to create a BLE server with custom service 
   and characteristic UUIDs.

   Key Features:
   - BLE server setup with custom UUIDs for service and characteristic.
   - Handling connections and disconnections to/from BLE clients.
   - Reading and parsing JSON data received from the client.
   - Sending data packets back to the client at regular intervals.

   The server sends updates on voltage, current, and output enable status at a defined 
   interval. It also processes incoming data to control the device's output, voltage, 
   and current limit based on client commands.

   Developed by Jason Too
   License: MIT

   MIT License
   Copyright (c) 2023 elektroThing

   Permission is hereby granted, free of charge, to any person obtaining a copy of 
   this software and associated documentation files (the "Software"), to deal in 
   the Software without restriction, including without limitation the rights to use, 
   copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
   Software, and to permit persons to whom the Software is furnished to do so, subject 
   to the following conditions:

   The above copyright notice and this permission notice shall be included in all 
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
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

bool isOutputOn;
int selectedVoltage;
int currentLimit;

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

        // Parsing the received JSON data
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, value.c_str());
        
        // Assign values from JSON to respective variables
        isOutputOn = doc["output"];
        selectedVoltage = doc["voltage"];
        currentLimit = doc["currentLimit"];
        
        // Debugging outputs
        Serial.print("Output is: "); Serial.println(isOutputOn ? "On" : "Off");
        Serial.print("Voltage: "); Serial.println(selectedVoltage);
        Serial.print("Current Limit: "); Serial.println(currentLimit);
        Serial.println();
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
