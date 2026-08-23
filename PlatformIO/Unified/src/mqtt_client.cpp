/**
 * mqtt_client.cpp - MQTT Client for Home Automation Integration Implementation
 */

#include "mqtt_client.h"
#include "config.h"
#include "config_manager.h"
#include "pd_pps_engine.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// External references
extern ConfigManager* configManager;
extern PDPPSEngine* pdppsEngine;
extern uint8_t systemStatus;
extern uint8_t sourceCapability;
extern int currentVoltage;
extern int currentLimit;
extern int currentDraw;
extern bool outputEnabled;

MQTTClient::MQTTClient() {
  mqttClient = nullptr;
  initialized = false;
  connected = false;
  mqttServer = "";
  mqttPort = MQTT_DEFAULT_PORT;
  mqttUser = "";
  mqttPassword = "";
  mqttClientId = "spark-analyzer";
  mqttBaseTopic = "spark-analyzer";
  lastConnectAttempt = 0;
  reconnectInterval = 5000;
}

void MQTTClient::begin() {
  Serial.println("Initializing MQTT client...");
  
  // Get MQTT configuration from config manager
  if (configManager) {
    mqttServer = configManager->getMQTTBroker();
    mqttPort = configManager->getMQTTPort();
    mqttUser = configManager->getMQTTUser();
    mqttPassword = configManager->getMQTTPassword();
    mqttBaseTopic = configManager->getMQTTBaseTopic();
  }
  
  // Set default values if not configured
  if (mqttServer.length() == 0) {
    mqttServer = "192.168.1.100"; // Default to common home assistant IP
  }
  
  // Generate unique client ID based on MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  mqttClientId = "spark-analyzer-" + String(mac[5], HEX);
  
  // Initialize MQTT client
  mqttClient = new AsyncMqttClient();
  
  // Set callbacks
  mqttClient->onConnect([this](bool sessionPresent) {
    this->onMqttConnect(sessionPresent);
  });
  
  mqttClient->onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
    this->onMqttDisconnect(reason);
  });
  
  mqttClient->onMessage([this](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    this->onMqttMessage(topic, payload, properties, len, index, total);
  });
  
  initialized = true;
  Serial.println("MQTT client initialized");
}

void MQTTClient::update() {
  if (!initialized) return;
  
  // Handle reconnection
  if (!connected && mqttServer.length() > 0) {
    if (millis() - lastConnectAttempt >= reconnectInterval) {
      connect();
      lastConnectAttempt = millis();
    }
  }
  
  // Publish periodic updates if connected
  if (connected) {
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish >= MQTT_UPDATE_INTERVAL_MS) {
      publishStatus();
      publishVoltage();
      publishCurrent();
      publishPower();
      lastPublish = millis();
    }
  }
}

void MQTTClient::connect() {
  if (!initialized || mqttServer.length() == 0) return;
  
  Serial.print("Connecting to MQTT broker: ");
  Serial.print(mqttServer);
  Serial.print(":");
  Serial.println(mqttPort);
  
  if (mqttUser.length() > 0) {
    mqttClient->setCredentials(mqttUser.c_str(), mqttPassword.c_str());
  }
  
  mqttClient->setClientId(mqttClientId.c_str());
  mqttClient->setServer(mqttServer.c_str(), mqttPort);
  
  mqttClient->connect();
}

void MQTTClient::disconnect() {
  if (mqttClient) {
    mqttClient->disconnect();
  }
}

bool MQTTClient::isConnected() {
  return connected;
}

void MQTTClient::onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT broker");
  connected = true;
  
  // Subscribe to command topics
  mqttClient->subscribe(getTopic("command/voltage").c_str(), MQTT_QOS);
  mqttClient->subscribe(getTopic("command/current").c_str(), MQTT_QOS);
  mqttClient->subscribe(getTopic("command/output").c_str(), MQTT_QOS);
  
  // Publish initial status
  publishStatus();
}

void MQTTClient::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.print("Disconnected from MQTT broker. Reason: ");
  Serial.println((int)reason);
  connected = false;
}

void MQTTClient::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.print("MQTT message received: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.println(payload);
  
  // Parse command from topic
  String topicStr = String(topic);
  
  if (topicStr.indexOf("command/voltage") >= 0) {
    int voltage = atoi(payload);
    if (pdppsEngine) {
      pdppsEngine->setVoltage(voltage);
    }
  } else if (topicStr.indexOf("command/current") >= 0) {
    int limit = atoi(payload);
    if (pdppsEngine) {
      pdppsEngine->setCurrentLimit(limit);
    }
  } else if (topicStr.indexOf("command/output") >= 0) {
    bool enabled = (strcmp(payload, "true") == 0 || strcmp(payload, "1") == 0);
    if (pdppsEngine) {
      pdppsEngine->setOutputEnabled(enabled);
    }
  }
}

void MQTTClient::publishStatus() {
  if (!connected) return;
  
  StaticJsonDocument<256> doc;
  doc["status"] = systemStatus;
  doc["source_capability"] = sourceCapability;
  doc["output_enabled"] = outputEnabled;
  doc["timestamp"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  mqttClient->publish(getTopic("status").c_str(), (uint8_t)MQTT_QOS, false, json.c_str(), json.length());
}

void MQTTClient::publishVoltage() {
  if (!connected) return;
  
  StaticJsonDocument<128> doc;
  doc["voltage"] = currentVoltage;
  doc["timestamp"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  mqttClient->publish(getTopic("voltage").c_str(), (uint8_t)MQTT_QOS, false, json.c_str(), json.length());
}

void MQTTClient::publishCurrent() {
  if (!connected) return;
  
  StaticJsonDocument<128> doc;
  doc["current"] = currentDraw;
  doc["current_limit"] = currentLimit;
  doc["timestamp"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  mqttClient->publish(getTopic("current").c_str(), (uint8_t)MQTT_QOS, false, json.c_str(), json.length());
}

void MQTTClient::publishPower() {
  if (!connected) return;
  
  StaticJsonDocument<128> doc;
  float power = (currentVoltage * currentDraw) / 1000.0;
  doc["power"] = power;
  doc["timestamp"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  mqttClient->publish(getTopic("power").c_str(), (uint8_t)MQTT_QOS, false, json.c_str(), json.length());
}

String MQTTClient::getTopic(const char* subtopic) {
  return mqttBaseTopic + "/" + String(subtopic);
}