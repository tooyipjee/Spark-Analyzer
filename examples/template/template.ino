/**
 *  Spark Analyzer Firmware for Custom Applications
 * 
 * This firmware for the Spark Analyzer platform integrates USB-C Power Delivery (PD) control and precise current monitoring. Designed for customization, it supports a broad range of applications from power management to IoT devices.
 * 
 * Features:
 * - Precision voltage regulation via USB-C PD, with support for multiple voltage levels.
 * - Accurate current measurement using a moving average filter, including automatic zero-error calibration.
 * - Configurable initial output state and adjustable monitoring frequency.
 * - Enhanced debugging and serial communication for real-time monitoring.
 * 
 * Customization Points:
 * - Voltage level selection for diverse application needs.
 * - Non-blocking current sensor updates ensure uninterrupted PD control.
 * - Clear integration points for developers to add application-specific logic.
 * 
 * Ideal for developers seeking a robust foundation for building electronic systems with advanced power management and monitoring requirements.
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

// Import required libraries
#include <Arduino.h>
#include <Wire.h>
#include <PD_UFP.h> // USB Power Delivery (PD) library for control over USB-C
#include "CurrentSensor.h" // Custom class for handling current sensing

// Define desired USB PD voltage setting
#define VOLTAGE PD_POWER_OPTION_MAX_5V
// Options:
// PD_POWER_OPTION_MAX_5V
// PD_POWER_OPTION_MAX_9V
// PD_POWER_OPTION_MAX_12V
// PD_POWER_OPTION_MAX_15V
// PD_POWER_OPTION_MAX_20V

// Timing for non-blocking updates
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 100; // Interval for current sensor updates in milliseconds
// Pin assignments - AVALIABLE PINS ARE 8, 9, 20 (RX)  & 21 (TX)
const int usb_pd_int_pin = 10; // USB PD interrupt pin
const int output_pin = 3; // Output control pin (e.g., to turn a device on/off)
const int current_pin = 2; // Analog pin for current sensor input
// Initialize objects for USB PD control and current sensing
CurrentSensor currentSensor(current_pin);
PD_UFP_c PD_UFP;

void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  Serial.println("Initializing...");
  // Setup pin modes for control signals and sensors
  pinMode(usb_pd_int_pin, INPUT);
  pinMode(output_pin, OUTPUT);
  digitalWrite(output_pin, LOW); // Initially set output to LOW (off) for calibration
  pinMode(current_pin, INPUT);
  // Calibrate the current sensor to account for zero error
  currentSensor.calibrateZeroError();
  digitalWrite(output_pin, HIGH); // Set output to HIGH (on) after calibration
  // Initialize I2C for USB PD control
  Wire.begin();
  Wire.setClock(400000); // Set I2C clock speed to 400kHz
  PD_UFP.init(usb_pd_int_pin, VOLTAGE); // Initialize USB PD with the selected voltage

  // ### User can add initialization code for other components here ###


}

void loop() {
  // Perform a non-blocking check to update the current sensor reading
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis(); // Update the time of the last update
    currentSensor.update(); // Perform the current sensor reading update

    // ### User can add code here to react to the updated current reading ###


  }
  // Continuously run PD_UFP to handle USB PD communication efficiently
  PD_UFP.run();

  // ### User can add general application code here that needs to run continuously ###


}
