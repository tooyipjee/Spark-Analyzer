/*
   -- Spark Analyzer GUI Firmware --
   
   This source code is for the graphical user interface of the Spark Analyzer, 
   a smart home device for monitoring and controlling power delivery. 
   The GUI is generated automatically by the RemoteXY editor. 
   The RemoteXY library (version 3.1.8 or later) is required for compiling this code, 
   which can be downloaded from http://remotexy.com/en/library/.
   To connect with the RemoteXY mobile app, visit http://remotexy.com/en/download/:
     - ANDROID version 4.11.1 or later;
     - iOS version 1.9.1 or later.
    
   This source code is free software; you can redistribute it and/or modify it under 
   the terms of the GNU Lesser General Public License as published by the Free Software 
   Foundation, version 2.1 or any later version.    

   Developed by Jason Too
   License: MIT

   MIT License
   Copyright (c) 2023 elektroThing
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of 
   this software and associated documentation files (the "Software"), to deal in 
   the Software without restriction, including without limitation the rights to use, 
   copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
   Software, and to permit persons to whom the Software is furnished to do so, subject 
   to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all 
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <Wire.h>
#include "tcpm_driver.h"
#include "usb_pd.h"
#include <Preferences.h>
#define FILTER_LENGTH 1000

//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>

#include <RemoteXY.h>

// RemoteXY connection settings 
#define REMOTEXY_BLUETOOTH_NAME "Spark Analyzer"


// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 124 bytes
  { 255,2,0,4,0,117,0,16,31,1,130,1,255,255,65,15,17,68,17,0,
  62,64,38,8,13,3,5,3,16,10,35,13,26,10,48,39,25,15,15,13,
  26,31,79,78,0,31,79,70,70,0,129,0,4,3,33,6,13,83,112,97,
  114,107,32,65,110,97,108,121,122,101,114,0,129,0,12,56,36,6,13,67,
  117,114,114,101,110,116,32,40,109,65,41,0,129,0,16,17,7,6,13,53,
  86,0,129,0,16,31,10,6,13,49,50,86,0,129,0,16,45,10,6,13,
  50,48,86,0 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t Voltage; // =0 if select position A, =1 if position B, =2 if position C, ... 
  uint8_t pushSwitch_1; // =1 if state is ON, else =0 

    // output variables
  float CurrentADCVal;

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)
/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////
#define MOVING_AVERAGE_LENGTH 10  // Length of the moving average filter
int adcSamples[MOVING_AVERAGE_LENGTH];
int adcIndex = 0;
int adcSum = 0;
int adcFiltered = 0;

Preferences preferences;
int setVoltage;
const int usb_pd_int_pin = 10;
const int debug_led_pin  = 3;
const int current_pin = 2;
int current = 0;
int voltage = 0;
int filterBuf = 0;
int adcError = 0;
int loopCount = 0;
int sampledVal = 0; 
// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};
int readFilteredADC(int pin);
void setup() 
{
  RemoteXY_Init (); 
  preferences.begin("credentials", false); 
  setVoltage = preferences.getInt("Voltage");
  RemoteXY.Voltage = setVoltage;

  Serial.begin(115200);

  pinMode(usb_pd_int_pin, INPUT);
  pinMode(debug_led_pin, OUTPUT);
  digitalWrite(debug_led_pin, HIGH);
  pinMode(current_pin, INPUT);

  Wire.begin(1,0);
  Wire.setClock(400000);
  delay(500);
  digitalWrite(debug_led_pin, LOW);
  tcpm_init(0);
  delay(50);
  pd_init(0);
  delay(50);

  if(RemoteXY.Voltage == 0)
  {
    pd_set_max_voltage(5000);  
  }
  else if(RemoteXY.Voltage == 1)
  {
    pd_set_max_voltage(9000);  
  }
  else if(RemoteXY.Voltage == 2)
  {
    pd_set_max_voltage(12000);  
  }
  else if(RemoteXY.Voltage == 3)
  {
    pd_set_max_voltage(15000);  
  }
  else if(RemoteXY.Voltage == 4)
  {
    pd_set_max_voltage(20000);  
  }

  // Initialize the filter buffer
  for (int i = 0; i < MOVING_AVERAGE_LENGTH; i++) {
    adcSamples[i] = analogRead(current_pin);
    adcSum += adcSamples[i];
    delay(1);  // Short delay to get different samples
  }
}

void loop() 
{ 
  RemoteXY_Handler ();

  if (RemoteXY.Voltage != setVoltage)
  {
    Serial.println("Voltage changed!");
    preferences.putInt("Voltage", RemoteXY.Voltage);
    preferences.end();
    ESP.restart();
  }
    if(RemoteXY.pushSwitch_1 == 1)
  {
    voltage = ((sampledVal-adcError)*1.0/4096.0)*3300;
    // Serial.println(voltage);

    current = (voltage - 1650)*10;
    // Serial.println(current);

    digitalWrite(debug_led_pin, HIGH);
  }
  else
  {
    adcError = sampledVal-2048;
    // Serial.println(adcError);
    digitalWrite(debug_led_pin,LOW);

  }

  if (LOW == digitalRead(usb_pd_int_pin)) {
      tcpc_alert(0);
  }

  pd_run_state_machine(0);

  // Current calculation
  if(RemoteXY.pushSwitch_1 == 1) {
    digitalWrite(debug_led_pin, HIGH);
  } else {
    adcError = readFilteredADC(current_pin);
    digitalWrite(debug_led_pin,LOW);
  }
  current = 5.6865*(readFilteredADC(current_pin)-adcError);

  RemoteXY.CurrentADCVal  = current;

  delay(4);

}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  digitalWrite(debug_led_pin, HIGH);

  Serial.print("Voltage set to ");
  Serial.println(PD_MAX_VOLTAGE_MV);
}

int readFilteredADC(int pin) {
  int newSample = analogRead(pin);
  adcSum -= adcSamples[adcIndex];  // Subtract the oldest sample
  adcSamples[adcIndex] = newSample; // Update the oldest sample with the new sample
  adcSum += newSample;  // Add the new sample to the sum
  adcIndex = (adcIndex + 1) % MOVING_AVERAGE_LENGTH; // Increment the index (and wrap around if necessary)
  return adcSum / MOVING_AVERAGE_LENGTH;  // Return the average
}