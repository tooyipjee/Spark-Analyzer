/*
   -- New project --
   
   This source code of graphical user interface 
   has been generated automatically by RemoteXY editor.
   To compile this code using RemoteXY library 3.1.10 or later version 
   download by link http://remotexy.com/en/library/
   To connect using RemoteXY mobile app by link http://remotexy.com/en/download/                   
     - for ANDROID 4.13.1 or later version;
     - for iOS 1.10.1 or later version;
    
   This source code is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.    
*/

//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// you can enable debug logging to Serial at 115200
//#define REMOTEXY__DEBUGLOG    

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>


// RemoteXY connection settings 
#define REMOTEXY_BLUETOOTH_NAME "Spark Analyzer"


#include <RemoteXY.h>

// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 112 bytes
  { 255,9,0,4,0,105,0,17,0,0,0,31,1,106,200,1,1,5,0,129,
  2,5,103,9,27,86,111,108,116,97,103,101,32,40,86,41,32,32,32,32,
  32,32,67,117,114,114,101,110,116,32,40,65,41,0,68,4,77,99,116,49,
  31,27,67,117,114,114,101,110,116,32,40,109,65,41,0,7,3,19,44,13,
  44,27,30,29,3,7,60,19,44,13,44,27,30,29,3,2,28,48,51,16,
  0,29,26,31,31,79,78,0,79,70,70,0 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  float voltage_set;
  float current_set;
  uint8_t output_enable; // =1 if switch ON and =0 if OFF

    // output variables
  float current_graph;

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;
#pragma pack(pop)
 
/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////


#include <Wire.h>
#include <PD_UFP.h>
#include <Arduino.h>

#define FUSB302_INT_PIN   10
#define OUTPUT_PIN 3
#define FILTER_LENGTH 10
const int current_pin = 2;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 100; // 500ms
int adcError = 0;
int adcIndex = 0;
int adcSum = 0;
int adcSamples[FILTER_LENGTH];

bool output = 1;
float current = 0;
PD_UFP_c PD_UFP;

void setup() {
  Wire.begin(1,0);
  PD_UFP.init_PPS(FUSB302_INT_PIN, PPS_V(12.0), PPS_A(1.0));
  
  Serial.begin(9600);
  pinMode(OUTPUT_PIN,OUTPUT);

  RemoteXY_Init (); 

    for(int i = 0; i < 30; i++)
  {
    processCurrentReading();

  }
}

void loop() {
  PD_UFP.run();
  RemoteXY_Handler ();
  PD_UFP.set_PPS(PPS_V(RemoteXY.voltage_set), PPS_A(RemoteXY.current_set));
  if (PD_UFP.is_PPS_ready())
  {
    Serial.write("PPS trigger succcess\n");
  }
  else if (PD_UFP.is_power_ready())
  {
    Serial.write("Fail to trigger PPS\n");
  }
  else
  {
    Serial.write("No PD supply available\n");
  }

  Serial.println(RemoteXY.voltage_set);
  Serial.println(RemoteXY.current_set);
  output = RemoteXY.output_enable;
  RemoteXY.current_graph = current;

  processCurrentReading();


}


// Process current reading and adjust LED status
void processCurrentReading() {
  if(output) {
    digitalWrite(OUTPUT_PIN, HIGH);
  } else {
    adcError = readFilteredADC(current_pin);
    digitalWrite(OUTPUT_PIN, LOW);
  }
  current = 5.6865 * (readFilteredADC(current_pin) - adcError);
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
