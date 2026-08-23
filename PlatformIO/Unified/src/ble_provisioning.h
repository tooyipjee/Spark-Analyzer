/**
 * ble_provisioning.h - Minimal Custom BLE WiFi Provisioning
 * 
 * Minimal BLE-based WiFi credential provisioning using ESP32 native BLE APIs
 * Replaces heavy NimBLE library with custom implementation for memory efficiency
 */

#ifndef BLE_PROVISIONING_H
#define BLE_PROVISIONING_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Provisioning states
enum ProvisioningState {
  PROV_IDLE,           // Not provisioning
  PROV_ADVERTISING,    // BLE advertising, waiting for connection
  PROV_CONNECTED,      // Device connected, waiting for credentials
  PROV_RECEIVING,      // Receiving credentials
  PROV_SUCCESS,        // Credentials received successfully
  PROV_FAILED          // Provisioning failed
};

class BLEProvisioning {
public:
  BLEProvisioning();
  void begin();
  void update();
  void startProvisioning();
  void stopProvisioning();
  ProvisioningState getState();
  bool isProvisioningComplete();
  
private:
  bool initialized;
  ProvisioningState state;
  
  // BLE objects (using ESP32 native BLE)
  BLEServer* bleServer;
  BLEService* bleService;
  BLECharacteristic* ssidCharacteristic;
  BLECharacteristic* passwordCharacteristic;
  BLECharacteristic* statusCharacteristic;
  
  // Provisioning data
  String receivedSSID;
  String receivedPassword;
  unsigned long advertisingStartTime;
  unsigned long connectionStartTime;
  
  // Callback classes
  class ServerCallbacks;
  class CharacteristicCallbacks;
  
  void setupBLE();
  void startAdvertising();
  void stopAdvertising();
  void handleReceivedCredentials();
  bool validateCredentials();
  void saveCredentials();
  void notifyStatus(const String& status);
};

#endif // BLE_PROVISIONING_H