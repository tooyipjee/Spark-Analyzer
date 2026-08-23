/**
 * pd_pps_engine.h - PD/PPS Auto-Detection and Control Engine
 * 
 * Manages USB-C Power Delivery and Programmable Power Supply:
 * - Automatic source capability detection
 * - Smart mode preference (PPS preferred over PD)
 * - Voltage and current control
 * - PD renegotiation handling
 */

#ifndef PD_PPS_ENGINE_H
#define PD_PPS_ENGINE_H

#include <Arduino.h>
#include "PD_UFP.h"

class PDPPSEngine {
public:
  PDPPSEngine();
  void begin();
  void update();
  
  // Source detection
  uint8_t getSourceCapability();
  bool isPPSAvailable();
  bool isPDReady();
  
  // Voltage control
  bool setVoltage(int voltage);
  int getCurrentVoltage();
  bool setPPSVoltage(float voltage); // For PPS fine control
  float getPPSVoltage();
  
  // Current control
  bool setCurrentLimit(int limit);
  int getCurrentLimit();
  int getCurrentDraw();
  
  // Output control
  bool setOutputEnabled(bool enabled);
  bool isOutputEnabled();
  
  // Mode management
  bool setModePreferPPS(bool prefer);
  bool getModePreferPPS();
  
private:
  PD_UFP_c pdUfp;
  uint8_t interruptPin;
  uint8_t sourceCapability;
  bool preferPPS;
  bool outputEnabled;
  int currentVoltage;
  int currentLimit;
  int currentDraw;
  bool initialized;
  
  void detectSourceCapability();
  bool negotiateVoltage(int voltage);
  bool negotiatePPSVoltage(float voltage);
  void updateCurrentMeasurement();
};

#endif // PD_PPS_ENGINE_H