#include <Wire.h>
#include <PD_UFP.h>

#define FUSB302_INT_PIN   10
#define OUTPUT_PIN 3
PD_UFP_c PD_UFP;

void setup() {
  Wire.begin(1,0);
  PD_UFP.init_PPS(FUSB302_INT_PIN, PPS_V(8.4), PPS_A(2.0));
  
  Serial.begin(9600);
  pinMode(OUTPUT_PIN,OUTPUT);
  digitalWrite(OUTPUT_PIN,HIGH);
}

void loop() {
  PD_UFP.run();
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
}