/**
 * pd_pps_engine.cpp - PD/PPS Auto-Detection and Control Engine Implementation
 */

#include "pd_pps_engine.h"
#include "config.h"
#include "led_controller.h"
#include "config_manager.h"

// External references
extern LEDController* ledController;
extern ConfigManager* configManager;

PDPPSEngine::PDPPSEngine() {
  interruptPin = USB_PD_INT_PIN;
  sourceCapability = SOURCE_NONE;
  preferPPS = true;
  outputEnabled = false;
  currentVoltage = DEFAULT_VOLTAGE;
  currentLimit = DEFAULT_CURRENT_LIMIT;
  currentDraw = 0;
  initialized = false;
}

void PDPPSEngine::begin() {
  Serial.println("Initializing PD/PPS engine...");
  
  // Initialize I2C for FUSB302
  Wire.begin(FUSB302_I2C_SDA, FUSB302_I2C_SCL);
  Wire.setClock(FUSB302_I2C_CLOCK);
  delay(500);
  
  // Initialize PD UFP library
  // Start with 5V default
  pdUfp.init(interruptPin, PD_POWER_OPTION_MAX_5V);
  
  // Allow time for PD negotiation
  delay(1000);
  
  // Detect source capabilities
  detectSourceCapability();
  
  initialized = true;
  Serial.println("PD/PPS engine initialized");
}

void PDPPSEngine::update() {
  if (!initialized) return;
  
  // Run PD state machine
  pdUfp.run();
  
  // Update current measurement
  updateCurrentMeasurement();
  
  // Check source capability periodically
  static unsigned long lastDetection = 0;
  if (millis() - lastDetection >= 5000) { // Check every 5 seconds
    lastDetection = millis();
    detectSourceCapability();
  }
}

uint8_t PDPPSEngine::getSourceCapability() {
  return sourceCapability;
}

bool PDPPSEngine::isPPSAvailable() {
  return sourceCapability == SOURCE_PPS_AVAILABLE || sourceCapability == SOURCE_PPS_ACTIVE;
}

bool PDPPSEngine::isPDReady() {
  return sourceCapability == SOURCE_PD_ONLY || sourceCapability == SOURCE_PPS_AVAILABLE || sourceCapability == SOURCE_PPS_ACTIVE;
}

bool PDPPSEngine::setVoltage(int voltage) {
  if (!initialized) return false;
  
  // Check if source supports this voltage
  if (!isPDReady()) {
    Serial.println("No PD source available");
    return false;
  }
  
  // If PPS is available and preferred, use PPS
  if (preferPPS && isPPSAvailable()) {
    return setPPSVoltage((float)voltage);
  }
  
  // Otherwise use PD negotiation
  return negotiateVoltage(voltage);
}

int PDPPSEngine::getCurrentVoltage() {
  if (!initialized) return DEFAULT_VOLTAGE;
  
  // Get voltage from PD library
  if (pdUfp.is_power_ready() || pdUfp.is_PPS_ready()) {
    currentVoltage = pdUfp.get_voltage() / 1000; // Convert from 50mV units to volts
  }
  
  return currentVoltage;
}

bool PDPPSEngine::setPPSVoltage(float voltage) {
  if (!initialized || !isPPSAvailable()) return false;
  
  // PPS voltage is in 20mV units
  uint16_t ppsVoltage = (uint16_t)(voltage * 50); // Convert to 20mV units
  
  // Set PPS voltage
  bool success = pdUfp.set_PPS(ppsVoltage, currentLimit / 50); // Current in 50mA units
  
  if (success) {
    sourceCapability = SOURCE_PPS_ACTIVE;
    currentVoltage = (int)voltage;
    Serial.print("PPS voltage set to ");
    Serial.print(voltage);
    Serial.println("V");
  }
  
  return success;
}

float PDPPSEngine::getPPSVoltage() {
  if (!initialized) return 0.0;
  
  if (pdUfp.is_PPS_ready()) {
    uint16_t ppsVoltage = pdUfp.get_voltage(); // In 20mV units
    return (float)ppsVoltage / 50.0; // Convert to volts
  }
  
  return (float)getCurrentVoltage();
}

bool PDPPSEngine::setCurrentLimit(int limit) {
  if (!initialized) return false;
  
  currentLimit = limit;
  
  // If PPS is active, update the current limit
  if (sourceCapability == SOURCE_PPS_ACTIVE) {
    uint16_t ppsVoltage = pdUfp.get_voltage();
    uint8_t ppsCurrent = limit / 50; // Convert to 50mA units
    return pdUfp.set_PPS(ppsVoltage, ppsCurrent);
  }
  
  return true;
}

int PDPPSEngine::getCurrentLimit() {
  return currentLimit;
}

int PDPPSEngine::getCurrentDraw() {
  return currentDraw;
}

bool PDPPSEngine::setOutputEnabled(bool enabled) {
  if (!initialized) return false;
  
  outputEnabled = enabled;
  
  // Control output via GPIO
  digitalWrite(OUTPUT_ENABLE_PIN, enabled ? HIGH : LOW);
  
  // Update LED to reflect output state
  if (ledController) {
    ledController->setOutputState(enabled);
  }
  
  Serial.print("Output ");
  Serial.println(enabled ? "enabled" : "disabled");
  
  return true;
}

bool PDPPSEngine::isOutputEnabled() {
  return outputEnabled;
}

bool PDPPSEngine::setModePreferPPS(bool prefer) {
  preferPPS = prefer;
  
  // If we're changing preference and current mode doesn't match, renegotiate
  if (prefer && sourceCapability == SOURCE_PD_ONLY) {
    // Can't prefer PPS if source doesn't support it
    return false;
  }
  
  if (!prefer && sourceCapability == SOURCE_PPS_ACTIVE) {
    // Switch back to PD mode
    return negotiateVoltage(currentVoltage);
  }
  
  return true;
}

bool PDPPSEngine::getModePreferPPS() {
  return preferPPS;
}

void PDPPSEngine::detectSourceCapability() {
  if (!initialized) return;
  
  // Check if power is ready
  if (pdUfp.is_power_ready()) {
    uint16_t voltage = pdUfp.get_voltage();
    uint16_t current = pdUfp.get_current();
    
    Serial.print("PD power ready: ");
    Serial.print(voltage / 50); // Convert to 50mV units for display
    Serial.print("V, ");
    Serial.print(current / 10); // Convert to 10mA units for display
    Serial.println("A");
    
    // Check if PPS is available
    if (pdUfp.is_PPS_ready()) {
      sourceCapability = SOURCE_PPS_AVAILABLE;
      Serial.println("PPS capability detected");
      
      // If we prefer PPS, automatically switch to PPS mode
      if (preferPPS) {
        setPPSVoltage((float)currentVoltage);
      }
    } else {
      sourceCapability = SOURCE_PD_ONLY;
      Serial.println("PD only mode");
    }
  } else {
    sourceCapability = SOURCE_NONE;
    Serial.println("No PD power available");
  }
}

bool PDPPSEngine::negotiateVoltage(int voltage) {
  if (!initialized) return false;
  
  // Map voltage to PD power option
  enum PD_power_option_t powerOption;
  
  switch (voltage) {
    case 5:
      powerOption = PD_POWER_OPTION_MAX_5V;
      break;
    case 9:
      powerOption = PD_POWER_OPTION_MAX_9V;
      break;
    case 12:
      powerOption = PD_POWER_OPTION_MAX_12V;
      break;
    case 15:
      powerOption = PD_POWER_OPTION_MAX_15V;
      break;
    case 20:
      powerOption = PD_POWER_OPTION_MAX_20V;
      break;
    default:
      Serial.println("Unsupported voltage for PD mode");
      return false;
  }
  
  // PD renegotiation may require restart
  // Update LED to indicate renegotiation
  if (ledController) {
    ledController->setPattern(LED_PATTERN_PD_RENEGOTIATING);
  }
  
  Serial.print("Negotiating PD voltage: ");
  Serial.print(voltage);
  Serial.println("V");
  
  // Reinitialize PD with new voltage option
  pdUfp.init(interruptPin, powerOption);
  
  // Wait for negotiation
  delay(1000);
  
  // Check if successful
  if (pdUfp.is_power_ready()) {
    currentVoltage = voltage;
    sourceCapability = SOURCE_PD_ONLY;
    Serial.println("PD voltage negotiation successful");
    
    // Update LED to ready state
    if (ledController) {
      ledController->setPattern(LED_PATTERN_PD_READY);
    }
    
    return true;
  } else {
    Serial.println("PD voltage negotiation failed");
    return false;
  }
}

bool PDPPSEngine::negotiatePPSVoltage(float voltage) {
  if (!initialized || !isPPSAvailable()) return false;
  
  return setPPSVoltage(voltage);
}

void PDPPSEngine::updateCurrentMeasurement() {
  if (!initialized) return;
  
  // Read current from ADC
  int adcValue = analogRead(CURRENT_SENSE_PIN);
  
  // Apply calibration and scaling
  // This is a simplified calculation - real implementation would use proper calibration
  static int adcError = 0;
  static int adcSamples[ADC_FILTER_LENGTH];
  static int adcIndex = 0;
  static int adcSum = 0;
  
  // Moving average filter
  adcSum -= adcSamples[adcIndex];
  adcSamples[adcIndex] = adcValue;
  adcSum += adcValue;
  adcIndex = (adcIndex + 1) % ADC_FILTER_LENGTH;
  
  int filteredADC = adcSum / ADC_FILTER_LENGTH;
  
  // Calculate current (simplified)
  if (!outputEnabled) {
    adcError = filteredADC; // Calibrate zero point when output is off
  }
  
  currentDraw = (int)(CURRENT_SENSOR_SCALE * (filteredADC - adcError));
  
  // Ensure current is not negative
  if (currentDraw < 0) currentDraw = 0;
}