/**
 * PlatformIO Template Firmware for USB-C Power Delivery and Current Monitoring
 * 
 * This firmware, designed for PlatformIO, serves as a template for developers creating custom applications 
 * involving USB-C Power Delivery (PD) and current monitoring. It is optimized for use in PlatformIO environment, 
 * enabling developers to leverage its advanced features for efficient development.
 * 
 * Key Features:
 * - Initialization and management of USB-C Power Delivery for precise voltage control.
 * - Real-time current measurement using a moving average filter for accurate readings.
 * - Configurable settings for initial output state and current limit.
 * - Serial debugging support for real-time monitoring and troubleshooting.
 * - Defined function prototypes for better code organization and readability.
 * - Update interval configuration for flexible monitoring and control frequency.
 * 
 * Developers can use this template as a starting point to build sophisticated applications requiring 
 * USB-C PD control and current monitoring, such as smart chargers, power management systems, 
 * and laboratory equipment.
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

// User-configurable constants
#define FILTER_LENGTH 10
#define INITIAL_OUTPUT_STATE 1 // 1 for On, 0 for Off
#define CURRENT_LIMIT 0        // Set to desired limit, 0 for no limit
#define VOLTAGE 5

// Filter variables
int adcSamples[FILTER_LENGTH];
int adcIndex = 0;
int adcSum = 0;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 100; // 500ms
const int usb_pd_int_pin = 10;
const int debug_led_pin  = 3;
const int current_pin = 2;
int current = 0;
bool output = INITIAL_OUTPUT_STATE;
int voltage = VOLTAGE;
int adcError = 0;
// Prototypes
// Function Prototypes
void initializeSerialAndPins();
void initializeUSB_PD();
void updateStatus();
void processCurrentReading();
void checkCurrentLimit();
int readFilteredADC(int pin);

// USB-C Specific - TCPM start
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};

void setup() {
  output = 0;
  initializeUSB_PD();
  for(int i = 0; i < 30; i++)
  {
    processCurrentReading();

  }
  initializeSerialAndPins();

  output = 1;
}


void loop() {
  updateStatus();
  processCurrentReading();
  checkCurrentLimit();
}



// Initialize Serial and Pin Modes
void initializeSerialAndPins() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  pinMode(usb_pd_int_pin, INPUT);
  pinMode(debug_led_pin, OUTPUT);
  digitalWrite(debug_led_pin, HIGH);
  pinMode(current_pin, INPUT);
}

// Initialize USB Power Delivery
void initializeUSB_PD() {
  Wire.begin(1, 0);
  Wire.setClock(400000);
  delay(500);
  tcpm_init(0);
  delay(50);
  pd_init(0);
  delay(50);
  pd_set_max_voltage(voltage*1000); // Set default 5V
}

// Update status at intervals
void updateStatus() {
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis();
    // Add any periodic update logic here
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

// // // Additional functions for USB PD or other functionalities can be added here
// #include <Arduino.h>

// void setup() {
//   Serial.begin(115200); // Initialize serial communication at 115200 baud rate
// }

// void loop() {
//   Serial.println("Hello world!"); // Print "Hello world!" to the Serial Monitor
//   delay(1000); // Wait for a second
// }
