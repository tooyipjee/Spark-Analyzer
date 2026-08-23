/**
 * button_handler.cpp - Multi-Function Button Handler Implementation
 */

#include "button_handler.h"
#include "config.h"
#include "led_controller.h"
#include "config_manager.h"

// External references to global modules
extern LEDController* ledController;
extern ConfigManager* configManager;

ButtonHandler::ButtonHandler() {
  buttonPin = PROGRAMMABLE_BUTTON_PIN;
  lastButtonState = HIGH;  // Assumes pull-up
  currentButtonState = HIGH;
  pressStartTime = 0;
  lastDebounceTime = 0;
  lastEvent = BUTTON_EVENT_NONE;
  confirmWindowActive = false;
  confirmWindowStartTime = 0;
}

void ButtonHandler::begin() {
  if (buttonPin >= 0) {
    pinMode(buttonPin, INPUT_PULLUP);
    lastButtonState = digitalRead(buttonPin);
    currentButtonState = lastButtonState;
  }
}

void ButtonHandler::update() {
  if (buttonPin < 0) {
    return; // Button not available
  }
  
  debounce();
  handlePress();
}

ButtonEvent ButtonHandler::getEvent() {
  ButtonEvent event = lastEvent;
  lastEvent = BUTTON_EVENT_NONE;
  return event;
}

void ButtonHandler::debounce() {
  bool reading = digitalRead(buttonPin);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) >= BUTTON_DEBOUNCE_MS) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
    }
  }
  
  lastButtonState = reading;
}

bool ButtonHandler::isPressed() {
  return currentButtonState == LOW; // Assumes pull-up
}

void ButtonHandler::handlePress() {
  if (isPressed()) {
    if (pressStartTime == 0) {
      pressStartTime = millis();
    }
    
    unsigned long pressDuration = millis() - pressStartTime;
    
    // Handle confirmation window for factory reset
    if (confirmWindowActive) {
      if (pressDuration >= FACTORY_RESET_PRESS_MS) {
        // Still holding after confirmation window - this is a valid factory reset
        confirmWindowActive = false;
        triggerFactoryReset();
        pressStartTime = 0;
      } else if ((millis() - confirmWindowStartTime) >= RESET_CONFIRM_WINDOW_MS) {
        // Confirmation window expired without completing
        confirmWindowActive = false;
        pressStartTime = 0;
        if (ledController) {
          ledController->setPattern(LED_PATTERN_IDLE);
        }
      }
      return;
    }
    
    // Check for factory reset threshold
    if (pressDuration >= FACTORY_RESET_PRESS_MS && !confirmWindowActive) {
      // Enter confirmation window
      confirmWindowActive = true;
      confirmWindowStartTime = millis();
      if (ledController) {
        ledController->setPattern(LED_PATTERN_SAFE_MODE); // Use distinctive pattern
      }
    }
    
  } else {
    // Button released
    if (pressStartTime > 0) {
      unsigned long pressDuration = millis() - pressStartTime;
      
      if (confirmWindowActive) {
        // Button released during confirmation window - this confirms factory reset
        confirmWindowActive = false;
        triggerFactoryReset();
      } else {
        // Normal button release - check duration
        if (pressDuration >= LONG_PRESS_MS) {
          triggerWiFiReset();
        } else if (pressDuration >= SHORT_PRESS_MS) {
          lastEvent = BUTTON_EVENT_SHORT_PRESS;
          if (ledController) {
            ledController->triggerBlinkPattern(1); // 1 blink for output toggle
          }
        }
      }
      
      pressStartTime = 0;
    }
  }
}

void ButtonHandler::triggerWiFiReset() {
  lastEvent = BUTTON_EVENT_LONG_PRESS;
  
  if (ledController) {
    ledController->triggerBlinkPattern(3); // 3 blinks for WiFi reset
  }
  
  // Clear WiFi credentials
  if (configManager) {
    configManager->clearWiFiCredentials();
  }
  
  // Trigger BLE provisioning again
  // This would be handled by the main system logic
}

void ButtonHandler::triggerFactoryReset() {
  lastEvent = BUTTON_EVENT_FACTORY_RESET;
  
  if (ledController) {
    ledController->triggerBlinkPattern(10); // 10 blinks for factory reset
  }
  
  // Perform factory reset
  if (configManager) {
    configManager->factoryReset();
  }
  
  // Restart the device
  ESP.restart();
}