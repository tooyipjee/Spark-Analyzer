/**
 * button_handler.h - Multi-Function Button Handler
 * 
 * Handles the programmable button with multiple functions:
 * - Short press: Toggle power output
 * - Long press (3s): WiFi reset
 * - Very long press (10s): Factory reset (with confirmation window)
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

// Button events
enum ButtonEvent {
  BUTTON_EVENT_NONE,
  BUTTON_EVENT_SHORT_PRESS,    // Output toggle
  BUTTON_EVENT_LONG_PRESS,     // WiFi reset
  BUTTON_EVENT_FACTORY_RESET   // Factory reset
};

class ButtonHandler {
public:
  ButtonHandler();
  void begin();
  void update();
  ButtonEvent getEvent();
  
private:
  int buttonPin;
  bool lastButtonState;
  bool currentButtonState;
  unsigned long pressStartTime;
  unsigned long lastDebounceTime;
  ButtonEvent lastEvent;
  bool confirmWindowActive;
  unsigned long confirmWindowStartTime;
  
  void debounce();
  bool isPressed();
  void handlePress();
  void triggerWiFiReset();
  void triggerFactoryReset();
};

#endif // BUTTON_HANDLER_H