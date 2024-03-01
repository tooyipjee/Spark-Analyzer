/**
 * Spark Analyzer - ESP-NOW Communication System
 * 
 * This firmware is designed for the Spark Analyzer system, utilizing ESP-NOW 
 * for wireless communication between a host (controller) and multiple peripherals (nodes).
 * 
 * Host Firmware:
 * - Sends voltage and enable/disable commands to peripherals.
 * - Manages multiple peripherals using their MAC addresses.
 * - Receives user input via serial interface for real-time control.
 * - Provides feedback on the status of data transmission.
 * 
 * Peripheral Firmware:
 * - Receives and processes commands from the host.
 * - Controls a NeoPixel LED strip based on the received data.
 * - Utilizes EEPROM for persistent storage of voltage setpoints.
 * - Manages USB-C Power Delivery and monitors current.
 * - Implements a moving average filter for stable current readings.
 * - Displays status and control information via NeoPixel LED effects.
 * 
 * This firmware set enables efficient and low-latency control of power and lighting systems,
 * suitable for smart home and IoT applications.
 * 
 * Developed by Jason Too
 * License: MIT
 * 
 * MIT License
 * Copyright (c) 2023 elektroThing
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Arduino.h>
#include <Wire.h>
#include "tcpm_driver.h"
#include "usb_pd.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h> // Include NeoPixel Library

// NeoPixel Constants
#define PIN 8
#define NUMPIXELS 175
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// User-configurable constants
#define FILTER_LENGTH 10
#define CURRENT_LIMIT 0        // Set to desired limit, 0 for no limit

// Filter variables
int adcSamples[FILTER_LENGTH];
int adcIndex = 0;
int adcSum = 0;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 100; // Update interval in ms
const int usb_pd_int_pin = 10;
const int debug_led_pin  = 3;
const int current_pin = 2;
int current = 0;
bool output = true;
int voltage = 5; // Default voltage
int adcError = 0;

// USB-C Specific - TCPM start
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};

// MAC addresses of the ESP boards (for board 1,2,3)
// uint8_t newMacAdr[] = {0x3C, 0x71, 0xBF, 0xC3, 0xBF, 0xB0};
// uint8_t newMacAdr[] = {0x24, 0x0A, 0xC4, 0xAE, 0xAE, 0x44};
uint8_t newMacAdr[] = {0x80, 0x7D, 0x3A, 0x58, 0xB4, 0xB0};

// Updated structure to match the sender's data structure
typedef struct {
  int voltage;
  bool enable;
} voltage_enable_struct;

// Create an instance of the structure to hold incoming data
voltage_enable_struct myData;

// Callback function executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Voltage: ");
  Serial.println(myData.voltage);
  Serial.print("Enable: ");
  Serial.println(myData.enable ? "True" : "False");
  Serial.println();
  voltage = myData.voltage;

    // Check and update voltage setpoint
  checkAndUpdateVoltageSetpoint(myData.voltage);
  output = myData.enable;
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  EEPROM.begin(512); // For ESP8266/ESP32, specify EEPROM size
  int storedVoltage = EEPROM.read(0); // Read from address 0
  if (storedVoltage != 255) { // Check if EEPROM is not empty (255 is default empty value)
    voltage = storedVoltage;
  }
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  // NeoPixel initialization
  pixels.begin();
  // Optionally set a custom MAC address for the Wi-Fi interface
  uint8_t newMacAddress[] = {0x24, 0x0A, 0xC4, 0xAE, 0xAE, 0x44};
  esp_wifi_set_mac(WIFI_IF_STA, newMacAddress);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback for receiving data
  esp_now_register_recv_cb(OnDataRecv);
  // Initialize Serial and Pin Modes
  pinMode(usb_pd_int_pin, INPUT);
  pinMode(debug_led_pin, OUTPUT);
  digitalWrite(debug_led_pin, output ? HIGH : LOW);
  pinMode(current_pin, INPUT);

  // Initialize USB Power Delivery
  Wire.begin(1, 0);
  Wire.setClock(400000);
  delay(500);
  tcpm_init(0);
  delay(50);
  pd_init(0);
  delay(50);
  pd_set_max_voltage(voltage * 1000); // Set default voltage
}

void loop() {
  updateStatus();
  processCurrentReading();
  checkCurrentLimit();
  fadeColors(); // NeoPixel color fading function

}

// Update status at intervals
void updateStatus() {
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis();
    // Print voltage and current
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print("V, Current (mA): ");
    Serial.println(current);
  }
  pd_run_state_machine(0);
}

// Process current reading and adjust LED status
void processCurrentReading() {
  if(output) {
    digitalWrite(debug_led_pin, HIGH);
  } else {
    adcError = readFilteredADC(current_pin);
    digitalWrite(debug_led_pin, LOW);
  }
  current = 5.6865 * (readFilteredADC(current_pin) - adcError);
}

// Check if current exceeds limit
void checkCurrentLimit() {
  if(current > CURRENT_LIMIT && CURRENT_LIMIT != 0) {
    output = 0;
  }
}

// Reads ADC value with a moving average filter
int readFilteredADC(int pin) {
    int newSample = analogRead(pin);
    adcSum -= adcSamples[adcIndex];
    adcSamples[adcIndex] = newSample;
    adcSum += newSample;
    adcIndex = (adcIndex + 1) % FILTER_LENGTH;
    return adcSum / FILTER_LENGTH;
}
void checkAndUpdateVoltageSetpoint(int newVoltage) {
  int storedVoltage = EEPROM.read(0); // Assuming the voltage setpoint is stored at address 0

  if (storedVoltage != newVoltage) {
    EEPROM.write(0, newVoltage);
    EEPROM.commit(); // For ESP8266/ESP32, to ensure data is written to EEPROM
    Serial.println("Voltage setpoint changed. Resetting device...");
    delay(1000); // Delay to allow serial message to be sent before resetting
    ESP.restart(); // Use this for ESP8266/ESP32
    // For other Arduino boards, use the following:
    // asm volatile ("  jmp 0");  
  }
}

// NeoPixel color fading function
void fadeColors() {
  static unsigned long lastUpdate = 0;
  const long interval = 50; // Time interval for color changes
  static int fadeValue = 0;
  static int stage = 0;

  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdate >= interval) {
    lastUpdate = currentMillis;

    switch (stage) {
      case 0: // Fade in blue
        setStripColor(0, 0, fadeValue);
        break;
      case 1: // Transition to green
        setStripColor(0, fadeValue, 255 - fadeValue);
        break;
      case 2: // Transition to red
        setStripColor(fadeValue, 255 - fadeValue, 0);
        break;
      case 3: // Transition to blue (from red)
        setStripColor(255 - fadeValue, 0, fadeValue);
        break;
      case 4: // Fade out to off
        setStripColor(0, 0, 255 - fadeValue);
        break;
    }

    pixels.show();
    fadeValue++;

    if (fadeValue > 255) {
      fadeValue = 0;
      stage++;
      if (stage > 4) stage = 0; // Reset to initial stage after completion
    }
  }
}

// Function to set NeoPixel strip color
void setStripColor(uint8_t red, uint8_t green, uint8_t blue) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
}
// Additional functions for USB PD or other functionalities can be added here