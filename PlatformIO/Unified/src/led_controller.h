/**
 * led_controller.h - LED Status Indication Controller
 * 
 * Manages the 3 LEDs on the Spark Analyzer for status indication:
 * - Power On LED: Hardware controlled (device powered)
 * - Output Enable LED: Output status (solid = on, off = disabled)
 * - Debug LED: Source capability and system status indication
 */

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

// LED pattern definitions
enum LEDPattern {
  LED_PATTERN_IDLE,           // No activity
  LED_PATTERN_BOOTING,        // System booting
  LED_PATTERN_PROVISIONING,   // WiFi provisioning in progress
  LED_PATTERN_CONNECTING,      // WiFi connecting
  LED_PATTERN_READY,          // System ready, no source
  LED_PATTERN_PD_DETECTING,   // Detecting PD source
  LED_PATTERN_PD_READY,       // PD source ready
  LED_PATTERN_PPS_READY,      // PPS source ready
  LED_PATTERN_PD_RENEGOTIATING, // PD renegotiation in progress
  LED_PATTERN_ERROR,          // System error
  LED_PATTERN_SAFE_MODE,      // Safe mode active
  LED_PATTERN_OUTPUT_TOGGLE,   // Output toggled (1 blink)
  LED_PATTERN_WIFI_RESET,      // WiFi reset (3 blinks)
  LED_PATTERN_FACTORY_RESET    // Factory reset (10 blinks)
};

class LEDController {
public:
  LEDController();
  void begin();
  void update();
  void setPattern(LEDPattern pattern);
  void setOutputState(bool enabled);
  void triggerBlinkPattern(uint8_t blinkCount);
  
private:
  LEDPattern currentPattern;
  bool outputEnabled;
  unsigned long patternStartTime;
  uint8_t patternStep;
  bool blinkPatternActive;
  uint8_t blinkCount;
  uint8_t blinkIndex;
  unsigned long lastBlinkTime;
  
  void updateHardware();
  void updatePattern();
  void updateBlinkPattern();
  void setDebugLED(bool state);
  void setOutputLED(bool state);
};

#endif // LED_CONTROLLER_H