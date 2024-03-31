#include <Wire.h>
#include <PD_UFP.h>

#define FUSB302_INT_PIN   10
#define OUTPUT_PIN        3

// Stepper motor control pins
#define DIR_PIN           8
#define STEP_PIN          9
#define EN_PIN            20  // Enable pin, if used

// Stepping control
#define STEP_DELAY        100  // Delay between steps in milliseconds, for slow stepping

PD_UFP_c PD_UFP;

void setup() {
  Wire.begin(1,0);
  PD_UFP.init_PPS(FUSB302_INT_PIN, PPS_V(8.4), PPS_A(2.0));
  
  Serial.begin(9600);
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, HIGH);

  // Initialize stepper motor pins
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Enable the driver

  digitalWrite(DIR_PIN, HIGH); // Set initial direction
  Serial.println("Setup complete, starting loop...");
}

void loop() {
  Serial.println("Attempting to step motor...");

  // Simple stepping loop
  for (int i = 0; i < 50000; i++) {  // Example: 200 steps
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(10);  // Short pulse to ensure step detection
    digitalWrite(STEP_PIN, LOW);
    delay(STEP_DELAY);  // Slow stepping to ensure motor can follow
    Serial.print("Step: ");
    Serial.println(i + 1);
  }

  Serial.println("Completed 50000 steps. Pausing...");
  delay(5000);  // Pause before repeating the steps

  // This loop will continuously repeat, stepping the motor every 5 seconds
}
