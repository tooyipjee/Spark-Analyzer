/**
 * config_manager.cpp - Configuration and Settings Manager Implementation
 */

#include "config_manager.h"
#include "config.h"

ConfigManager::ConfigManager() {
  initialized = false;
}

void ConfigManager::begin() {
  preferences.begin("spark_analyzer", false);
  initialized = true;
}

// WiFi configuration
bool ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
  if (!initialized) return false;
  
  preferences.putString(ConfigKeys::KEY_WIFI_SSID, ssid);
  preferences.putString(ConfigKeys::KEY_WIFI_PASSWORD, password);
  return true;
}

String ConfigManager::getWiFiSSID() {
  if (!initialized) return "";
  return preferences.getString(ConfigKeys::KEY_WIFI_SSID, "");
}

String ConfigManager::getWiFiPassword() {
  if (!initialized) return "";
  return preferences.getString(ConfigKeys::KEY_WIFI_PASSWORD, "");
}

void ConfigManager::clearWiFiCredentials() {
  if (!initialized) return;
  
  preferences.remove(ConfigKeys::KEY_WIFI_SSID);
  preferences.remove(ConfigKeys::KEY_WIFI_PASSWORD);
}

// System configuration
void ConfigManager::setHostname(const String& hostname) {
  if (!initialized) return;
  preferences.putString(ConfigKeys::KEY_HOSTNAME, hostname);
}

String ConfigManager::getHostname() {
  if (!initialized) return DEFAULT_HOSTNAME;
  return preferences.getString(ConfigKeys::KEY_HOSTNAME, DEFAULT_HOSTNAME);
}

void ConfigManager::setPassword(const String& password) {
  if (!initialized) return;
  preferences.putString(ConfigKeys::KEY_PASSWORD, password);
}

String ConfigManager::getPassword() {
  if (!initialized) return DEFAULT_PASSWORD;
  return preferences.getString(ConfigKeys::KEY_PASSWORD, DEFAULT_PASSWORD);
}

// Power settings
void ConfigManager::setDefaultVoltage(int voltage) {
  if (!initialized) return;
  preferences.putInt(ConfigKeys::KEY_DEFAULT_VOLTAGE, voltage);
}

int ConfigManager::getDefaultVoltage() {
  if (!initialized) return DEFAULT_VOLTAGE;
  return preferences.getInt(ConfigKeys::KEY_DEFAULT_VOLTAGE, DEFAULT_VOLTAGE);
}

void ConfigManager::setDefaultCurrentLimit(int limit) {
  if (!initialized) return;
  preferences.putInt(ConfigKeys::KEY_DEFAULT_CURRENT_LIMIT, limit);
}

int ConfigManager::getDefaultCurrentLimit() {
  if (!initialized) return DEFAULT_CURRENT_LIMIT;
  return preferences.getInt(ConfigKeys::KEY_DEFAULT_CURRENT_LIMIT, DEFAULT_CURRENT_LIMIT);
}

// Boot tracking
void ConfigManager::incrementBootCount() {
  if (!initialized) return;
  
  uint32_t count = getBootCount();
  preferences.putUInt(ConfigKeys::KEY_BOOT_COUNT, count + 1);
}

uint32_t ConfigManager::getBootCount() {
  if (!initialized) return 0;
  return preferences.getUInt(ConfigKeys::KEY_BOOT_COUNT, 0);
}

void ConfigManager::resetBootCount() {
  if (!initialized) return;
  preferences.putUInt(ConfigKeys::KEY_BOOT_COUNT, 0);
}

// MQTT configuration
void ConfigManager::setMQTTEnabled(bool enabled) {
  if (!initialized) return;
  preferences.putBool(ConfigKeys::KEY_MQTT_ENABLED, enabled);
}

bool ConfigManager::isMQTTEnabled() {
  if (!initialized) return MQTT_ENABLED;
  return preferences.getBool(ConfigKeys::KEY_MQTT_ENABLED, MQTT_ENABLED);
}

void ConfigManager::setMQTTBroker(const String& broker) {
  if (!initialized) return;
  preferences.putString(ConfigKeys::KEY_MQTT_BROKER, broker);
}

String ConfigManager::getMQTTBroker() {
  if (!initialized) return "";
  return preferences.getString(ConfigKeys::KEY_MQTT_BROKER, "");
}

void ConfigManager::setMQTTPort(int port) {
  if (!initialized) return;
  preferences.putInt(ConfigKeys::KEY_MQTT_PORT, port);
}

int ConfigManager::getMQTTPort() {
  if (!initialized) return MQTT_DEFAULT_PORT;
  return preferences.getInt(ConfigKeys::KEY_MQTT_PORT, MQTT_DEFAULT_PORT);
}

void ConfigManager::setMQTTCredentials(const String& user, const String& password) {
  if (!initialized) return;
  preferences.putString(ConfigKeys::KEY_MQTT_USER, user);
  preferences.putString(ConfigKeys::KEY_MQTT_PASS, password);
}

String ConfigManager::getMQTTUser() {
  if (!initialized) return "";
  return preferences.getString(ConfigKeys::KEY_MQTT_USER, "");
}

String ConfigManager::getMQTTPassword() {
  if (!initialized) return "";
  return preferences.getString(ConfigKeys::KEY_MQTT_PASS, "");
}

void ConfigManager::setMQTTBaseTopic(const String& topic) {
  if (!initialized) return;
  preferences.putString("mqtt_topic", topic);
}

String ConfigManager::getMQTTBaseTopic() {
  if (!initialized) return "spark-analyzer";
  return preferences.getString("mqtt_topic", "spark-analyzer");
}

// API configuration
void ConfigManager::setAPIEnabled(bool enabled) {
  if (!initialized) return;
  preferences.putBool(ConfigKeys::KEY_API_ENABLED, enabled);
}

bool ConfigManager::isAPIEnabled() {
  if (!initialized) return true;
  return preferences.getBool(ConfigKeys::KEY_API_ENABLED, true);
}

// Factory reset
void ConfigManager::factoryReset() {
  if (!initialized) return;
  
  preferences.clear();
  resetBootCount();
}

void ConfigManager::networkReset() {
  if (!initialized) return;
  
  clearWiFiCredentials();
  resetBootCount();
}