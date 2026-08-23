/**
 * ota_manager.h - OTA Update Manager
 * 
 * Over-the-air firmware update manager with security checks
 * Supports HTTP and HTTPS firmware updates
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>

class OTAManager {
public:
  OTAManager();
  void begin();
  void update();
  
  bool startUpdate(const String& url);
  bool checkForUpdate(const String& versionUrl);
  String getCurrentVersion();
  String getLatestVersion();
  
private:
  bool initialized;
  bool updating;
  String currentVersion;
  String latestVersion;
  
  // OTA callbacks
  void onUpdateStart();
  void onUpdateProgress(size_t current, size_t total);
  void onUpdateEnd();
  void onUpdateError(int error);
  
  // Helper functions
  bool downloadAndInstall(const String& url);
  bool validateFirmware(const String& url);
};

#endif // OTA_MANAGER_H