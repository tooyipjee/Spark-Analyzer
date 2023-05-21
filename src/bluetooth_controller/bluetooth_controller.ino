/*
   -- PowerPod --
   
   This source code of graphical user interface 
   has been generated automatically by RemoteXY editor.
   To compile this code using RemoteXY library 3.1.8 or later version 
   download by link http://remotexy.com/en/library/
   To connect using RemoteXY mobile app by link http://remotexy.com/en/download/                   
     - for ANDROID 4.11.1 or later version;
     - for iOS 1.9.1 or later version;
    
   This source code is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.    
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
#define REMOTEXY_BLUETOOTH_NAME "PicoTin"


// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 58 bytes
  { 255,2,0,4,0,51,0,16,31,1,68,17,0,62,64,38,8,13,3,132,
  13,47,35,10,13,26,10,48,23,22,15,15,13,26,31,79,78,0,31,79,
  70,70,0,129,0,20,3,33,6,13,80,105,99,111,84,105,110,0 };
  
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
void setup() 
{
  RemoteXY_Init (); 
  preferences.begin("credentials", false); 
  setVoltage = preferences.getInt("Voltage");
  RemoteXY.Voltage = setVoltage;

  Serial.begin(115200);
  Serial.println(RemoteXY.Voltage);

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
    pd_set_max_voltage(15000);  
  }
  else if(RemoteXY.Voltage == 3)
  {
    pd_set_max_voltage(20000);  
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
  if(loopCount < FILTER_LENGTH)
  {
    filterBuf = filterBuf+analogRead(current_pin);
    loopCount++;
    
  }
  else
  {
    sampledVal = filterBuf/FILTER_LENGTH;

    loopCount = 0;
    filterBuf = 0;
    Serial.print("Sampled: ");
    Serial.println(sampledVal);
  }

  if(RemoteXY.pushSwitch_1 == 1)
  {
    voltage = ((sampledVal-adcError)*1.0/4096.0)*3300;
    Serial.println(voltage);

    current = (voltage - 1650)*10;
    Serial.println(current);

    digitalWrite(debug_led_pin, HIGH);
  }
  else
  {
    adcError = sampledVal-2048;
    Serial.println(adcError);
    digitalWrite(debug_led_pin,LOW);

  }

  if (LOW == digitalRead(usb_pd_int_pin)) {
      tcpc_alert(0);
  }

  pd_run_state_machine(0);


  RemoteXY.CurrentADCVal  = current;

  
  // Serial.print(pd_get_max_voltage());
  // Serial.print(", ");
  // Serial.println(RemoteXY.CurrentADCVal);

  // For some reason, a delay of 4 ms seems to be best
  // My guess is that spamming the I2C bus too fast causes problems
  // delay(4);

}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  digitalWrite(debug_led_pin, HIGH);

  Serial.print("Voltage set to ");
  Serial.println(PD_MAX_VOLTAGE_MV);
}