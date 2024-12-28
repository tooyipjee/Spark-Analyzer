#include <Arduino.h>
#include <Wire.h>
#include <PD_UFP.h>
#include "CurrentSensor.h"

#define VOLTAGE PD_POWER_OPTION_MAX_9V

// Timing for non-blocking updates
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 100;

// Pin assignments for ESP32-C3
const int usb_pd_int_pin = 10;  // Example GPIO pin, adjust as needed
const int output_pin = 3;       // Example GPIO pin, adjust as needed
const int current_pin = 2;      // ADC pin, adjust as needed
// const int stepper_dir_pin = 21; // TX pin (GPIO21) for direction control
// const int stepper_step_pin = 20; // RX pin (GPIO20) for step control
const int stepper_dir_pin = 20; // TX pin (GPIO21) for direction control
const int stepper_step_pin = 21; // RX pin (GPIO20) for step control

// Stepper motor control variables
const int stepsPerRevolution = 200;  // Adjust this based on your stepper motor
unsigned long lastStepTime = 0;
const unsigned long stepInterval = 1;  // Adjust for desired speed (1ms = 1000 steps/second)
int stepCount = 0;
bool motorDirection = true;  // true for clockwise, false for counterclockwise

// Debug print timing
unsigned long lastDebugPrintTime = 0;
const unsigned long debugPrintInterval = 1000;  // Print debug info every second

// Initialize objects
CurrentSensor currentSensor(current_pin);
PD_UFP_c PD_UFP;

void setup() {
  // Initialize serial communication on a different UART or disable it
  // Serial.begin(115200); // Commented out as we're using RX/TX for motor control

  // Use a different method for debug output, e.g., Serial1 if available
  Serial1.begin(115200); // Example using GPIO18 for RX and GPIO19 for TX
  Serial1.println("Initializing ESP32-C3 Spark Analyzer Firmware with Stepper Motor Control...");
  
  pinMode(usb_pd_int_pin, INPUT);
  pinMode(output_pin, OUTPUT);
  pinMode(current_pin, INPUT);
  pinMode(stepper_dir_pin, OUTPUT);
  pinMode(stepper_step_pin, OUTPUT);
  
  Serial1.println("Calibrating current sensor...");
  digitalWrite(output_pin, LOW);
  currentSensor.calibrateZeroError();
  digitalWrite(output_pin, HIGH);
  Serial1.println("Current sensor calibration complete.");
  
  Wire.begin(1,0);
  Wire.setClock(400000);
  PD_UFP.init_PPS(usb_pd_int_pin, PPS_V(12.0), PPS_A(2.0));
  Serial1.println("USB PD initialized.");

  // Initialize stepper motor pins
  digitalWrite(stepper_dir_pin, motorDirection ? HIGH : LOW);
  digitalWrite(stepper_step_pin, LOW);
  Serial1.println("Stepper motor pins initialized.");
  
  Serial1.println("Setup complete. Starting main loop...");
}

void loop() {
  // Current sensor update
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis();
    currentSensor.update();
  }

  // Stepper motor control
  if (millis() - lastStepTime >= stepInterval) {
    lastStepTime = millis();
    
    // Toggle the step pin
    digitalWrite(stepper_step_pin, HIGH);
    delayMicroseconds(10);  // Short delay for the step pulse
    digitalWrite(stepper_step_pin, LOW);
    
    stepCount++;
    
    // Change direction every revolution
    if (stepCount >= stepsPerRevolution) {
      stepCount = 0;
      motorDirection = !motorDirection;
      digitalWrite(stepper_dir_pin, motorDirection ? HIGH : LOW);
      Serial1.println("Stepper motor direction changed.");
    }
  }

  // Debug prints
  if (millis() - lastDebugPrintTime >= debugPrintInterval) {
    lastDebugPrintTime = millis();
    
    // Print current reading
    Serial1.print("Current: ");
    Serial1.print(currentSensor.getCurrent());
    Serial1.println(" mA");
    
    // Print stepper motor status
    Serial1.print("Stepper Motor - Steps: ");
    Serial1.print(stepCount);
    Serial1.print(", Direction: ");
    Serial1.println(motorDirection ? "Clockwise" : "Counterclockwise");
    
    // Print USB PD status
    Serial1.print("USB PD Voltage: ");
    Serial1.print(PD_UFP.get_voltage());
    Serial1.println(" mV");
    Serial1.print("USB PD Current: ");
    Serial1.print(PD_UFP.get_current());
    Serial1.println(" mA");
    
    Serial1.println("--------------------");
  }

  // Run USB PD communication
  PD_UFP.run();
}