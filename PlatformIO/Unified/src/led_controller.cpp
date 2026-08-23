/**
 * led_controller.cpp - LED Status Indication Controller Implementation
 */

#include "led_controller.h"
#include "config.h"

LEDController::LEDController() {
  currentPattern = LED_PATTERN_IDLE;
  outputEnabled = false;
  patternStartTime = 0;
  patternStep = 0;
  blinkPatternActive = false;
  blinkCount = 0;
  blinkIndex = 0;
  lastBlinkTime = 0;
}

void LEDController::begin() {
  // Initialize LED pins
  pinMode(DEBUG_LED_PIN, OUTPUT);
  pinMode(OUTPUT_ENABLE_PIN, OUTPUT);
  
  // Set initial states
  setDebugLED(false);
  setOutputLED(false);
  
  currentPattern = LED_PATTERN_BOOTING;
  patternStartTime = millis();
}

void LEDController::update() {
  updateBlinkPattern();
  updatePattern();
  updateHardware();
}

void LEDController::setPattern(LEDPattern pattern) {
  currentPattern = pattern;
  patternStartTime = millis();
  patternStep = 0;
}

void LEDController::setOutputState(bool enabled) {
  outputEnabled = enabled;
  setOutputLED(enabled);
}

void LEDController::triggerBlinkPattern(uint8_t count) {
  blinkPatternActive = true;
  blinkCount = count;
  blinkIndex = 0;
  lastBlinkTime = millis();
}

void LEDController::updateHardware() {
  // Update output LED based on current state
  setOutputLED(outputEnabled);
}

void LEDController::updatePattern() {
  if (blinkPatternActive) {
    return; // Let blink pattern take precedence
  }
  
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - patternStartTime;
  
  bool debugLEDState = false;
  unsigned long errorCycle = 0;
  unsigned long safeCycle = 0;
  unsigned long blinkPosition = 0;
  
  switch (currentPattern) {
    case LED_PATTERN_IDLE:
      debugLEDState = false;
      break;
      
    case LED_PATTERN_BOOTING:
      // Slow blink during booting
      debugLEDState = ((elapsedTime / LED_BLINK_LONG_MS) % 2) == 0;
      break;
      
    case LED_PATTERN_PROVISIONING:
      // Fast blink during provisioning
      debugLEDState = ((elapsedTime / LED_BLINK_SHORT_MS) % 2) == 0;
      break;
      
    case LED_PATTERN_CONNECTING:
      // Medium blink during connecting
      debugLEDState = ((elapsedTime / (LED_BLINK_SHORT_MS * 2)) % 2) == 0;
      break;
      
    case LED_PATTERN_READY:
      // Solid when ready, no source
      debugLEDState = true;
      break;
      
    case LED_PATTERN_PD_DETECTING:
      // Slow blink when detecting PD source
      debugLEDState = ((elapsedTime / LED_BLINK_LONG_MS) % 2) == 0;
      break;
      
    case LED_PATTERN_PD_READY:
      // Solid when PD source ready
      debugLEDState = true;
      break;
      
    case LED_PATTERN_PPS_READY:
      // Fast blink when PPS source ready (distinguishes from PD)
      debugLEDState = ((elapsedTime / LED_BLINK_SHORT_MS) % 2) == 0;
      break;
      
    case LED_PATTERN_PD_RENEGOTIATING:
      // Very fast blink during PD renegotiation
      debugLEDState = ((elapsedTime / (LED_BLINK_SHORT_MS / 2)) % 2) == 0;
      break;
      
    case LED_PATTERN_ERROR:
      // Long on, short off pattern for error
      errorCycle = elapsedTime % (LED_BLINK_LONG_MS + LED_BLINK_SHORT_MS);
      debugLEDState = errorCycle < LED_BLINK_LONG_MS;
      break;
      
    case LED_PATTERN_SAFE_MODE:
      // Triple blink pattern for safe mode
      safeCycle = elapsedTime % (3 * LED_BLINK_SHORT_MS * 2 + LED_PATTERN_DELAY_MS);
      blinkPosition = safeCycle % (LED_BLINK_SHORT_MS * 2);
      debugLEDState = (blinkPosition < LED_BLINK_SHORT_MS) && (safeCycle < (3 * LED_BLINK_SHORT_MS * 2));
      break;
      
    default:
      debugLEDState = false;
  }
  
  setDebugLED(debugLEDState);
}

void LEDController::updateBlinkPattern() {
  if (!blinkPatternActive) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  if (blinkIndex >= blinkCount * 2) {
    // Pattern complete
    blinkPatternActive = false;
    setDebugLED(false);
    return;
  }
  
  if (currentTime - lastBlinkTime >= LED_BLINK_SHORT_MS) {
    lastBlinkTime = currentTime;
    
    // Toggle LED state
    bool ledState = (blinkIndex % 2) == 0;
    setDebugLED(ledState);
    
    blinkIndex++;
  }
}

void LEDController::setDebugLED(bool state) {
  digitalWrite(DEBUG_LED_PIN, state ? HIGH : LOW);
}

void LEDController::setOutputLED(bool state) {
  digitalWrite(OUTPUT_ENABLE_PIN, state ? HIGH : LOW);
}