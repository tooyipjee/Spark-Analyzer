/**
 * ota_manager.cpp - OTA Update Manager Implementation
 */

#include "ota_manager.h"
#include "config.h"
#include <WiFi.h>

// External references
extern bool wifiConnected;

OTAManager::OTAManager() {
  initialized = false;
  updating = false;
  currentVersion = FIRMWARE_VERSION;
  latestVersion = "";
}

void OTAManager::begin() {
  Serial.println("Initializing OTA manager...");
  
  // Set up OTA update callbacks
  Update.onProgress([this](size_t current, size_t total) {
    this->onUpdateProgress(current, total);
  });
  
  initialized = true;
  Serial.println("OTA manager initialized");
  Serial.print("Current firmware version: ");
  Serial.println(currentVersion);
}

void OTAManager::update() {
  // OTA updates are event-driven, not periodic
  // This function can be used for periodic update checks
}

bool OTAManager::startUpdate(const String& url) {
  if (!initialized || updating) {
    Serial.println("OTA update not available or already in progress");
    return false;
  }
  
  if (!wifiConnected) {
    Serial.println("WiFi not connected, cannot perform OTA update");
    return false;
  }
  
  Serial.print("Starting OTA update from: ");
  Serial.println(url);
  
  updating = true;
  onUpdateStart();
  
  bool success = downloadAndInstall(url);
  
  if (success) {
    onUpdateEnd();
    Serial.println("OTA update successful, restarting...");
    delay(1000);
    ESP.restart();
  } else {
    onUpdateError(-1);
    updating = false;
    Serial.println("OTA update failed");
  }
  
  return success;
}

bool OTAManager::checkForUpdate(const String& versionUrl) {
  if (!initialized || !wifiConnected) {
    return false;
  }
  
  Serial.print("Checking for update from: ");
  Serial.println(versionUrl);
  
  HTTPClient http;
  http.begin(versionUrl);
  http.setTimeout(10000);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    latestVersion = http.getString();
    latestVersion.trim();
    
    Serial.print("Latest version: ");
    Serial.println(latestVersion);
    Serial.print("Current version: ");
    Serial.println(currentVersion);
    
    if (latestVersion != currentVersion) {
      Serial.println("Update available!");
      return true;
    } else {
      Serial.println("Already up to date");
      return false;
    }
  } else {
    Serial.print("Failed to check for update: ");
    Serial.println(httpCode);
    return false;
  }
  
  http.end();
}

String OTAManager::getCurrentVersion() {
  return currentVersion;
}

String OTAManager::getLatestVersion() {
  return latestVersion;
}

void OTAManager::onUpdateStart() {
  Serial.println("OTA update started");
  // Could trigger LED pattern here
}

void OTAManager::onUpdateProgress(size_t current, size_t total) {
  int progress = (current * 100) / total;
  Serial.print("OTA progress: ");
  Serial.print(progress);
  Serial.println("%");
  // Could update LED pattern here
}

void OTAManager::onUpdateEnd() {
  Serial.println("OTA update completed");
}

void OTAManager::onUpdateError(int error) {
  Serial.print("OTA update error: ");
  Serial.println(error);
}

bool OTAManager::downloadAndInstall(const String& url) {
  HTTPClient http;
  http.begin(url);
  http.setTimeout(30000);
  
  int httpCode = http.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
  
  int contentLength = http.getSize();
  Serial.print("Firmware size: ");
  Serial.print(contentLength);
  Serial.println(" bytes");
  
  if (contentLength <= 0) {
    Serial.println("Invalid content length");
    http.end();
    return false;
  }
  
  // Get stream for reading
  WiFiClient* stream = http.getStreamPtr();
  
  // Begin update
  if (!Update.begin(contentLength)) {
    Serial.println("Not enough space for update");
    http.end();
    return false;
  }
  
  // Download and write to flash
  uint8_t buffer[1024];
  size_t written = 0;
  
  while (written < contentLength) {
    size_t available = stream->available();
    if (available > 0) {
      size_t toRead = min(available, sizeof(buffer));
      size_t read = stream->readBytes(buffer, toRead);
      
      if (Update.write(buffer, read) != read) {
        Serial.println("Write failed");
        http.end();
        Update.abort();
        return false;
      }
      
      written += read;
      onUpdateProgress(written, contentLength);
    }
    delay(1);
  }
  
  http.end();
  
  // Finalize update
  if (!Update.end(true)) {
    Serial.println("Update end failed");
    return false;
  }
  
  return true;
}

bool OTAManager::validateFirmware(const String& url) {
  // Placeholder for firmware validation
  // Could check MD5 hash, signature, etc.
  return true;
}