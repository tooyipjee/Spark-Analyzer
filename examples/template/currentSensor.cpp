#include "CurrentSensor.h"

CurrentSensor::CurrentSensor(int pin) : sensorPin(pin), readIndex(0), total(0), zeroError(0), current(0.0f) {
    for (int i = 0; i < numReadings; i++) readings[i] = 0;
}

void CurrentSensor::calibrateZeroError(int numSamples) {
    long sum = 0;
    for (int i = 0; i < numSamples; i++) {
        int reading = analogRead(sensorPin);
        Serial.print("Calibration Reading ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(reading);
        sum += reading;
        delay(10); // Short delay to avoid rapid sampling
    }
    zeroError = sum / numSamples;
    Serial.print("Calibrated Zero Error: ");
    Serial.println(zeroError);
}

void CurrentSensor::update() {
    int newReading = analogRead(sensorPin);

    newReading -= zeroError;

    addReading(newReading);
    current = (total / (float)numReadings) * 5.6865;
    Serial.print("Current (mA): ");
    Serial.println(current);
}

void CurrentSensor::addReading(int reading) {
    total -= readings[readIndex];
    readings[readIndex] = reading;
    total += reading;
    readIndex = (readIndex + 1) % numReadings;
}

float CurrentSensor::getCurrent() {
    return current;
}
