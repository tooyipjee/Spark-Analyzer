/**
 * mqtt_client.h - MQTT Client for Home Automation Integration
 * 
 * MQTT client for real-time data streaming and home automation integration
 * Compatible with Home Assistant, Node-RED, and other MQTT systems
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

class MQTTClient {
public:
  MQTTClient();
  void begin();
  void update();
  void connect();
  void disconnect();
  bool isConnected();
  
private:
  AsyncMqttClient* mqttClient;
  bool initialized;
  bool connected;
  
  String mqttServer;
  uint16_t mqttPort;
  String mqttUser;
  String mqttPassword;
  String mqttClientId;
  String mqttBaseTopic;
  
  // Timer for reconnection
  unsigned long lastConnectAttempt;
  unsigned long reconnectInterval;
  
  // Callbacks
  void onMqttConnect(bool sessionPresent);
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
  
  // Publishing functions
  void publishStatus();
  void publishVoltage();
  void publishCurrent();
  void publishPower();
  
  // Topic generation
  String getTopic(const char* subtopic);
};

#endif // MQTT_CLIENT_H