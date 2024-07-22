// CurrentSensor.h
#ifndef CURRENTSENSOR_H
#define CURRENTSENSOR_H

#include <Arduino.h>

class CurrentSensor {
public:
    CurrentSensor(int pin);
    void calibrateZeroError(int numSamples = 50);
    void update();
    float getCurrent();

private:
    const int sensorPin;
    static const int numReadings = 10;
    int readings[numReadings]; // Circular buffer for readings
    int readIndex; // Current position in the buffer
    long total; // Sum of the readings
    int zeroError; // Zero error calibration value
    float current; // Last calculated current value

    void addReading(int reading);
};

#endif
