/**
 * config_manager.h - Configuration and Settings Manager
 * 
 * Manages device configuration using ESP32 Preferences (NVS):
 * - WiFi credentials
 * - System settings
 * - Source profiles
 * - Boot failure tracking
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// Configuration keys
namespace ConfigKeys {
  const char KEY_WIFI_SSID[] = "wifi_ssid";
  const char KEY_WIFI_PASSWORD[] = "wifi_pass";
  const char KEY_HOSTNAME[] = "hostname";
  const char KEY_PASSWORD[] = "password";
  const char KEY_DEFAULT_VOLTAGE[] = "def_voltage";
  const char KEY_DEFAULT_CURRENT_LIMIT[] = "def_current";
  const char KEY_BOOT_COUNT[] = "boot_count";
  const char KEY_MQTT_ENABLED[] = "mqtt_enabled";
  const char KEY_MQTT_BROKER[] = "mqtt_broker";
  const char KEY_MQTT_PORT[] = "mqtt_port";
  const char KEY_MQTT_USER[] = "mqtt_user";
  const char KEY_MQTT_PASS[] = "mqtt_pass";
  const char KEY_API_ENABLED[] = "api_enabled";
}

class ConfigManager {
public:
  ConfigManager();
  void begin();
  
  // WiFi configuration
  bool setWiFiCredentials(const String& ssid, const String& password);
  String getWiFiSSID();
  String getWiFiPassword();
  void clearWiFiCredentials();
  
  // System configuration
  void setHostname(const String& hostname);
  String getHostname();
  void setPassword(const String& password);
  String getPassword();
  
  // Power settings
  void setDefaultVoltage(int voltage);
  int getDefaultVoltage();
  void setDefaultCurrentLimit(int limit);
  int getDefaultCurrentLimit();
  
  // Boot tracking
  void incrementBootCount();
  uint32_t getBootCount();
  void resetBootCount();
  
  // MQTT configuration
  void setMQTTEnabled(bool enabled);
  bool isMQTTEnabled();
  void setMQTTBroker(const String& broker);
  String getMQTTBroker();
  void setMQTTPort(int port);
  int getMQTTPort();
  void setMQTTCredentials(const String& user, const String& password);
  String getMQTTUser();
  String getMQTTPassword();
  void setMQTTBaseTopic(const String& topic);
  String getMQTTBaseTopic();
  
  // API configuration
  void setAPIEnabled(bool enabled);
  bool isAPIEnabled();
  
  // Factory reset
  void factoryReset();
  void networkReset();
  
private:
  Preferences preferences;
  bool initialized;
};

#endif // CONFIG_MANAGER_H