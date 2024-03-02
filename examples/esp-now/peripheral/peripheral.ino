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
#include <PD_UFP.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <EEPROM.h>

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

PD_UFP_c PD_UFP;

// MAC addresses of the ESP boards
uint8_t newMacAdr[] = {0x80, 0x7D, 0x3A, 0x58, 0xB4, 0xB0};

typedef struct {
  int voltage;
  bool enable;
} voltage_enable_struct;

voltage_enable_struct myData;

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
  output = myData.enable;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  int storedVoltage = EEPROM.read(0);
  if (storedVoltage != 255) {
    voltage = storedVoltage;
  }

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  pinMode(usb_pd_int_pin, INPUT);
  pinMode(debug_led_pin, OUTPUT);
  digitalWrite(debug_led_pin, output ? HIGH : LOW);
  pinMode(current_pin, INPUT);

  Wire.begin(1, 0);
  Wire.setClock(400000);
  PD_UFP.init(usb_pd_int_pin, PD_POWER_OPTION_MAX_20V);
}

void loop() {
  updateStatus();
  processCurrentReading();
  checkCurrentLimit();
}

void updateStatus() {
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis();
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print("V, Current (mA): ");
    Serial.println(current);
  }
  PD_UFP.run();
}

void processCurrentReading() {
  if(output) {
    digitalWrite(debug_led_pin, HIGH);
  } else {
    adcError = readFilteredADC(current_pin);
    digitalWrite(debug_led_pin, LOW);
  }
  current = 5.6865 * (readFilteredADC(current_pin) - adcError);
}

void checkCurrentLimit() {
  if(current > CURRENT_LIMIT && CURRENT_LIMIT != 0) {
    output = 0;
  }
}

int readFilteredADC(int pin) {
    int newSample = analogRead(pin);
    adcSum -= adcSamples[adcIndex];
    adcSamples[adcIndex] = newSample;
    adcSum += newSample;
    adcIndex = (adcIndex + 1) % FILTER_LENGTH;
    return adcSum / FILTER_LENGTH;
}
