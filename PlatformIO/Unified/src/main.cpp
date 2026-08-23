/**
 * Spark Analyzer Unified Firmware - Main Application
 * 
 * Single firmware for all Spark Analyzer functionality:
 * - USB-C Power Delivery (PD) and Programmable Power Supply (PPS)
 * - Automatic source detection and optimal mode selection
 * - BLE WiFi provisioning
 * - Web interface with full features
 * - REST + MQTT APIs
 * - Advanced OTA updates
 * - Professional LED feedback
 * 
 * Developed by Jason Too
 * License: MIT
 * 
 * MIT License
 * Copyright (c) 2023 elektroThing
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "config.h"
#include "PD_UFP.h"
#include "led_controller.h"
#include "button_handler.h"
#include "config_manager.h"
#include "data_logger.h"
#include "pd_pps_engine.h"
#include "web_server.h"
#include "mqtt_client.h"
#include "ota_manager.h"

// Global instances
LEDController* ledController = nullptr;
ButtonHandler* buttonHandler = nullptr;
ConfigManager* configManager = nullptr;
DataLogger* dataLogger = nullptr;
PDPPSEngine* pdppsEngine = nullptr;
WebServer* webServer = nullptr;
MQTTClient* mqttClient = nullptr;
OTAManager* otaManager = nullptr;

// System state
uint8_t systemStatus = STATUS_BOOTING;
uint8_t sourceCapability = SOURCE_NONE;
bool outputEnabled = INITIAL_OUTPUT_STATE;
int currentVoltage = DEFAULT_VOLTAGE;
int currentLimit = DEFAULT_CURRENT_LIMIT;
int currentDraw = 0;
bool wifiConnected = false;

// Timing variables
unsigned long lastUpdateTime = 0;
unsigned long lastCurrentSample = 0;
unsigned long lastDataLog = 0;
unsigned long bootTime = 0;

// Function prototypes
void initializeHardware();
void initializeModules();
void connectToWiFi();
void updateSystem();
void handleSystemStatus();
void checkBootFailure();
void enterSafeMode();

void setup() {
  bootTime = millis();
  
  // Initialize serial communication first
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== Spark Analyzer Unified Firmware ===");
  Serial.println("Version: 1.0.0");
  Serial.println("Booting...");
  
  // Check for boot failure and potential safe mode
  checkBootFailure();
  
  // Initialize hardware peripherals
  initializeHardware();
  
  // Initialize all software modules
  initializeModules();
  
  // Connect to WiFi (or start BLE provisioning)
  connectToWiFi();
  
  systemStatus = STATUS_READY;
  Serial.println("System ready!");
  Serial.print("Boot time: ");
  Serial.print(millis() - bootTime);
  Serial.println("ms");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update system at regular intervals
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL_MS) {
    lastUpdateTime = currentTime;
    updateSystem();
  }
  
  // Sample current at regular intervals
  if (currentTime - lastCurrentSample >= CURRENT_SAMPLE_INTERVAL_MS) {
    lastCurrentSample = currentTime;
    // Current sampling will be handled by PD/PPS engine
  }
  
  // Log data at regular intervals
  if (currentTime - lastDataLog >= DATA_LOG_INTERVAL_MS) {
    lastDataLog = currentTime;
    if (dataLogger) {
      dataLogger->logData(currentVoltage, currentDraw, outputEnabled);
    }
  }
  
  // Handle system status LED patterns
  handleSystemStatus();
  
  // Give time to other tasks
  delay(10);
}

void initializeHardware() {
  Serial.println("Initializing hardware...");
  
  // Initialize I2C for FUSB302
  Wire.begin(FUSB302_I2C_SDA, FUSB302_I2C_SCL);
  Wire.setClock(FUSB302_I2C_CLOCK);
  Serial.println("I2C initialized");
  
  // Initialize GPIO pins
  pinMode(USB_PD_INT_PIN, INPUT);
  pinMode(OUTPUT_ENABLE_PIN, OUTPUT);
  pinMode(CURRENT_SENSE_PIN, INPUT);
  pinMode(DEBUG_LED_PIN, OUTPUT);
  
  if (PROGRAMMABLE_BUTTON_PIN >= 0) {
    pinMode(PROGRAMMABLE_BUTTON_PIN, INPUT_PULLUP);
  }
  
  // Set initial pin states
  digitalWrite(OUTPUT_ENABLE_PIN, outputEnabled ? HIGH : LOW);
  digitalWrite(DEBUG_LED_PIN, LOW);
  
  Serial.println("GPIO pins initialized");
}

void initializeModules() {
  Serial.println("Initializing software modules...");
  
  // Initialize LED controller first (for status feedback)
  ledController = new LEDController();
  ledController->begin();
  ledController->setPattern(LED_PATTERN_BOOTING);
  Serial.println("LED controller initialized");
  
  // Initialize configuration manager
  configManager = new ConfigManager();
  configManager->begin();
  Serial.println("Configuration manager initialized");
  
  // Initialize data logger
  dataLogger = new DataLogger();
  dataLogger->begin();
  Serial.println("Data logger initialized");
  
  // Initialize PD/PPS engine
  pdppsEngine = new PDPPSEngine();
  pdppsEngine->begin();
  Serial.println("PD/PPS engine initialized");
  
  // Initialize button handler
  if (PROGRAMMABLE_BUTTON_PIN >= 0) {
    buttonHandler = new ButtonHandler();
    buttonHandler->begin();
    Serial.println("Button handler initialized");
  }
  
  // Initialize web server
  if (FEATURE_WEB_SERVER) {
    webServer = new WebServer();
    webServer->begin();
    Serial.println("Web server initialized");
  }
  
  // Initialize MQTT client
  if (FEATURE_MQTT) {
    mqttClient = new MQTTClient();
    mqttClient->begin();
    Serial.println("MQTT client initialized");
  }
  
  // Initialize OTA manager
  if (FEATURE_OTA) {
    otaManager = new OTAManager();
    otaManager->begin();
    Serial.println("OTA manager initialized");
  }
  
  Serial.println("All modules initialized");
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  
  // Check if we have saved WiFi credentials
  String savedSSID = configManager->getWiFiSSID();
  String savedPassword = configManager->getWiFiPassword();
  
  if (savedSSID.length() > 0) {
    Serial.print("Found saved credentials for: ");
    Serial.println(savedSSID);
    
    // Try to connect to WiFi
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    unsigned long wifiConnectStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - wifiConnectStart >= WIFI_TIMEOUT_MS) {
        Serial.println("WiFi connection timeout");
        wifiConnected = false;
        return;
      }
      delay(500);
      Serial.print(".");
    }
    
    Serial.println();
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
    
    // Reset boot count on successful connection
    if (configManager) {
      configManager->resetBootCount();
    }
    
    // Start web server after WiFi connection
    if (webServer) {
      Serial.println("Web server ready on WiFi");
    }
    
    // Start MQTT client after WiFi connection
    if (mqttClient) {
      mqttClient->connect();
      Serial.println("MQTT client connecting...");
    }
    
  } else {
    Serial.println("No saved WiFi credentials found");
    wifiConnected = false;
  }
}

void updateSystem() {
  // Update all modules
  if (pdppsEngine) {
    pdppsEngine->update();
    sourceCapability = pdppsEngine->getSourceCapability();
    currentVoltage = pdppsEngine->getCurrentVoltage();
    currentDraw = pdppsEngine->getCurrentDraw();
  }
  
  if (ledController) {
    ledController->update();
  }
  
  if (buttonHandler) {
    buttonHandler->update();
  }
  
  if (webServer) {
    webServer->update();
  }
  
  if (mqttClient) {
    mqttClient->update();
  }
  
  if (otaManager) {
    otaManager->update();
  }
}

void handleSystemStatus() {
  // Update LED patterns based on system status
  LEDPattern pattern = LED_PATTERN_IDLE;
  
  switch (systemStatus) {
    case STATUS_BOOTING:
      pattern = LED_PATTERN_BOOTING;
      break;
    case STATUS_PROVISIONING:
      pattern = LED_PATTERN_PROVISIONING;
      break;
    case STATUS_CONNECTING:
      pattern = LED_PATTERN_CONNECTING;
      break;
    case STATUS_READY:
      pattern = LED_PATTERN_READY;
      break;
    case STATUS_PD_DETECTING:
      pattern = LED_PATTERN_PD_DETECTING;
      break;
    case STATUS_PD_READY:
      pattern = LED_PATTERN_PD_READY;
      break;
    case STATUS_PPS_READY:
      pattern = LED_PATTERN_PPS_READY;
      break;
    case STATUS_ERROR:
      pattern = LED_PATTERN_ERROR;
      break;
    case STATUS_SAFE_MODE:
      pattern = LED_PATTERN_SAFE_MODE;
      break;
    default:
      pattern = LED_PATTERN_IDLE;
  }
  
  if (ledController) {
    ledController->setPattern(pattern);
  }
}

void checkBootFailure() {
  // Check if this is a consecutive boot failure
  uint32_t bootCount = 0;
  if (configManager) {
    bootCount = configManager->getBootCount();
  }
  
  if (bootCount >= BOOT_FAILURE_THRESHOLD) {
    Serial.println("Boot failure threshold reached, entering safe mode");
    enterSafeMode();
  } else {
    // Increment boot counter
    if (configManager) {
      configManager->incrementBootCount();
    }
  }
}

void enterSafeMode() {
  Serial.println("Entering safe mode...");
  systemStatus = STATUS_SAFE_MODE;
  
  // Safe mode: minimal functionality for recovery
  // Only basic web server and LED feedback
  if (ledController) {
    ledController->setPattern(LED_PATTERN_SAFE_MODE);
  }
  
  // Keep system in safe mode for debugging/recovery
  while (true) {
    if (ledController) {
      ledController->update();
    }
    
    // In safe mode, check for factory reset button combination
    // or provide minimal web interface for recovery
    
    delay(100);
  }
}