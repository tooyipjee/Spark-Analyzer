/**
 * ble_provisioning.cpp - Minimal Custom BLE WiFi Provisioning Implementation
 */

#include "ble_provisioning.h"
#include "config.h"
#include "config_manager.h"
#include "led_controller.h"

// External references
extern ConfigManager* configManager;
extern LEDController* ledController;

// BLE Server Callbacks
class BLEProvisioning::ServerCallbacks : public BLEServerCallbacks {
public:
  ServerCallbacks(BLEProvisioning* parent) : parent(parent) {}
  
  void onConnect(BLEServer* pServer) {
    parent->state = PROV_CONNECTED;
    parent->connectionStartTime = millis();
    Serial.println("BLE device connected");
    parent->notifyStatus("CONNECTED");
  }
  
  void onDisconnect(BLEServer* pServer) {
    Serial.println("BLE device disconnected");
    if (parent->state == PROV_CONNECTED || parent->state == PROV_RECEIVING) {
      // If we disconnected before receiving credentials, go back to advertising
      parent->state = PROV_ADVERTISING;
      parent->startAdvertising();
    }
  }
  
private:
  BLEProvisioning* parent;
};

// Characteristic Callbacks
class BLEProvisioning::CharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
  CharacteristicCallbacks(BLEProvisioning* parent, bool isSSID) 
    : parent(parent), isSSID(isSSID) {}
  
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue();
    
    if (isSSID) {
      parent->receivedSSID = value;
      Serial.print("Received SSID: ");
      Serial.println(parent->receivedSSID);
    } else {
      parent->receivedPassword = value;
      Serial.println("Received password (length: " + String(value.length()) + ")");
    }
    
    parent->state = PROV_RECEIVING;
    parent->handleReceivedCredentials();
  }
  
private:
  BLEProvisioning* parent;
  bool isSSID;
};

BLEProvisioning::BLEProvisioning() {
  initialized = false;
  state = PROV_IDLE;
  bleServer = nullptr;
  bleService = nullptr;
  ssidCharacteristic = nullptr;
  passwordCharacteristic = nullptr;
  statusCharacteristic = nullptr;
  advertisingStartTime = 0;
  connectionStartTime = 0;
}

void BLEProvisioning::begin() {
  Serial.println("Initializing minimal BLE provisioning...");
  
  setupBLE();
  initialized = true;
  Serial.println("Minimal BLE provisioning initialized");
}

void BLEProvisioning::update() {
  if (!initialized) return;
  
  // Check advertising timeout
  if (state == PROV_ADVERTISING) {
    if (millis() - advertisingStartTime >= BLE_ADVERTISING_TIMEOUT_MS) {
      Serial.println("BLE advertising timeout, stopping provisioning");
      stopProvisioning();
    }
  }
  
  // Check connection timeout
  if (state == PROV_CONNECTED) {
    if (millis() - connectionStartTime >= 300000) { // 5 minute connection timeout
      Serial.println("BLE connection timeout, disconnecting");
      bleServer->disconnect(0);
    }
  }
}

void BLEProvisioning::startProvisioning() {
  if (!initialized) {
    Serial.println("BLE provisioning not initialized");
    return;
  }
  
  Serial.println("Starting BLE provisioning");
  state = PROV_ADVERTISING;
  receivedSSID = "";
  receivedPassword = "";
  
  if (ledController) {
    ledController->setPattern(LED_PATTERN_PROVISIONING);
  }
  
  startAdvertising();
}

void BLEProvisioning::stopProvisioning() {
  Serial.println("Stopping BLE provisioning");
  state = PROV_IDLE;
  stopAdvertising();
  
  if (ledController) {
    ledController->setPattern(LED_PATTERN_IDLE);
  }
}

ProvisioningState BLEProvisioning::getState() {
  return state;
}

bool BLEProvisioning::isProvisioningComplete() {
  return state == PROV_SUCCESS;
}

void BLEProvisioning::setupBLE() {
  // Initialize BLE device with name
  BLEDevice::init(BLE_DEVICE_NAME);
  
  // Set BLE power
  BLEDevice::setPower(ESP_PWR_LVL_P9); // +9dBm
  
  // Create BLE server
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks(this));
  
  // Create BLE service
  bleService = bleServer->createService(BLE_SERVICE_UUID);
  
  // Create SSID characteristic
  ssidCharacteristic = bleService->createCharacteristic(
    BLE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  ssidCharacteristic->setCallbacks(new CharacteristicCallbacks(this, true));
  
  // Create password characteristic (different UUID)
  passwordCharacteristic = bleService->createCharacteristic(
    "beb5483e-36e1-4688-b7f5-ea07361b26a9",
    BLECharacteristic::PROPERTY_WRITE
  );
  passwordCharacteristic->setCallbacks(new CharacteristicCallbacks(this, false));
  
  // Create status characteristic
  statusCharacteristic = bleService->createCharacteristic(
    "beb5483e-36e1-4688-b7f5-ea07361b26aa",
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  statusCharacteristic->addDescriptor(new BLE2902());
  
  // Start the service
  bleService->start();
  
  Serial.println("Minimal BLE server setup complete");
}

void BLEProvisioning::startAdvertising() {
  // Get advertising object and set data
  BLEAdvertising* pAdvertising = bleServer->getAdvertising();
  pAdvertising->addServiceUUID(bleService->getUUID());
  pAdvertising->setScanResponse(true);
  
  // Start advertising
  pAdvertising->start();
  advertisingStartTime = millis();
  
  Serial.println("Minimal BLE advertising started");
  notifyStatus("ADVERTISING");
}

void BLEProvisioning::stopAdvertising() {
  if (bleServer) {
    bleServer->getAdvertising()->stop();
    Serial.println("Minimal BLE advertising stopped");
  }
}

void BLEProvisioning::handleReceivedCredentials() {
  // Check if we have both SSID and password
  if (receivedSSID.length() > 0 && receivedPassword.length() > 0) {
    Serial.println("Received complete credentials");
    
    if (validateCredentials()) {
      saveCredentials();
      state = PROV_SUCCESS;
      notifyStatus("SUCCESS");
      
      // Stop advertising and disconnect
      stopAdvertising();
      bleServer->disconnect(0);
      
      if (ledController) {
        ledController->setPattern(LED_PATTERN_READY);
      }
      
      Serial.println("BLE provisioning successful");
    } else {
      state = PROV_FAILED;
      notifyStatus("INVALID_CREDENTIALS");
      Serial.println("Invalid credentials received");
    }
  }
}

bool BLEProvisioning::validateCredentials() {
  // Basic validation
  if (receivedSSID.length() < 1 || receivedSSID.length() > 32) {
    Serial.println("Invalid SSID length");
    return false;
  }
  
  if (receivedPassword.length() > 64) {
    Serial.println("Invalid password length");
    return false;
  }
  
  // Check for valid characters (basic ASCII check)
  for (int i = 0; i < receivedSSID.length(); i++) {
    if (receivedSSID[i] < 32 || receivedSSID[i] > 126) {
      Serial.println("Invalid character in SSID");
      return false;
    }
  }
  
  for (int i = 0; i < receivedPassword.length(); i++) {
    if (receivedPassword[i] < 32 || receivedPassword[i] > 126) {
      Serial.println("Invalid character in password");
      return false;
    }
  }
  
  return true;
}

void BLEProvisioning::saveCredentials() {
  if (configManager) {
    bool success = configManager->setWiFiCredentials(receivedSSID, receivedPassword);
    if (success) {
      Serial.println("WiFi credentials saved successfully");
    } else {
      Serial.println("Failed to save WiFi credentials");
    }
  }
}

void BLEProvisioning::notifyStatus(const String& status) {
  if (statusCharacteristic) {
    statusCharacteristic->setValue(status.c_str());
    statusCharacteristic->notify();
  }
}