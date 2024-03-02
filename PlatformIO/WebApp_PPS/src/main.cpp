/*
   NOTICE:
   This firmware for the Web App is developed using PlatformIO, an open-source ecosystem
   for IoT development. It leverages the SPIFFS (SPI Flash File System) for handling
   on-device file storage. Ensure that PlatformIO and necessary libraries are installed
   before compiling and uploading this firmware to your device.

   -- Web App Firmware --

   This script is the firmware for the Web App, designed to provide a user interface
   for monitoring and controlling power delivery in smart home applications. Key features
   include real-time current monitoring, voltage adjustment, and power output control.

   Key components and libraries:
     - ESPAsyncWebServer: For handling HTTP requests and serving the web interface.
     - EEPROM: To store and retrieve persistent settings such as the voltage level.
     - WiFiManager: To facilitate easy WiFi connectivity and configuration.
     - SPIFFS: For storing and accessing web interface files.

   Instructions for Loading the Firmware and Web Interface:
   1. Open the project in PlatformIO.
   2. Place the HTML file inside the 'data' directory of your PlatformIO project.
   3. Compile the firmware using PlatformIO. Ensure all library dependencies are resolved.
   4. Upload the firmware to your device using PlatformIO's upload functionality.
   5. Use PlatformIO's "Upload Filesystem Image" command to upload the contents of the 'data' directory
      to your device. This step ensures that the HTML file is stored in the SPIFFS.
   6. When done - connect your Spark Analyzer to your local WiFi using your phone/computer and searching for "Spark Analyzer" access point.
   7. Navigate to 192.168.4.1, and follow the web app to setup the device on your WiFi.
   8. Open serial terminal and the Spark Analyzer will print out it's IP address, navigate to that IP address and you should see the web app.

   Developed by Jason Too
   License: MIT

   MIT License
   Copyright (c) 2023 elektroThing

   Permission is hereby granted, free of charge, to any person obtaining a copy of this
   software and associated documentation files (the "Software"), to deal in the
   Software without restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so, subject to the
   following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Arduino.h>
#include <Wire.h>
#include <PD_UFP.h>
#include <WiFi.h>
#include <WiFiManager.h> // Include the WiFiManager library
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <EEPROM.h> // Include EEPROM library

void initializeSerialAndPins();
void initializeUSB_PD();
void updateStatus();
void processCurrentReading();
int readFilteredADC(int pin);

// User-configurable constants
#define FILTER_LENGTH 10
#define INITIAL_OUTPUT_STATE 0 // 1 for On, 0 for Off

// Filter variables
int adcSamples[FILTER_LENGTH];
int adcIndex = 0;
int adcSum = 0;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 100; // Set sample rate
const int usb_pd_int_pin = 10;
const int output_pin = 3;
const int current_pin = 2;
const int debug_led = 8;
int current = 0; // in mA
bool output = INITIAL_OUTPUT_STATE;
float voltage = 5;
float currentSet = 1;// in Ampere
int adcError = 0;

PD_UFP_c PD_UFP;

void handleCurrentChange(AsyncWebServerRequest *request) {
  if (request->hasParam("current")) {
    float newCurrent = request->getParam("current")->value().toFloat();
    if (newCurrent != currentSet) {
      currentSet = newCurrent;
      Serial.print("Current changed to ");
      Serial.println(currentSet);
      PD_UFP.set_PPS(PPS_V(voltage), PPS_A(currentSet));
      request->send(200, "text/plain", "Current limit updated");
    } else {
      request->send(200, "text/plain", "Current limit unchanged");
    }
  } else {
    request->send(400, "text/plain", "Current parameter missing");
  }
}
void handleVoltageChange(AsyncWebServerRequest *request)
{
  if (request->hasParam("voltage"))
  {
    float newVoltage = request->getParam("voltage")->value().toFloat();
    if (newVoltage != voltage)
    {
     
      voltage = newVoltage;
      Serial.print("Voltage changed to ");
      Serial.println(voltage);
      // esp_restart();                  // Restart ESP32-C3 to apply new voltage setting
      PD_UFP.set_PPS(PPS_V(voltage), PPS_A(currentSet));
    }
    else
    {
      request->send(200, "text/plain", "Voltage unchanged");
    }
    request->send(200, "text/plain", String(voltage));
  }
  else
  {
    request->send(400, "text/plain", "Voltage parameter missing");
  }
}

void handleOutputControl(AsyncWebServerRequest *request)
{
  if (request->hasParam("output"))
  {
    String outputState = request->getParam("output")->value();
    if (outputState == "1")
    {
      output = true;
      digitalWrite(output_pin, HIGH); // Turn output ON
      request->send(200, "text/plain", "Output Enabled");
    }
    else if (outputState == "0")
    {
      output = false;
      digitalWrite(output_pin, LOW); // Turn output OFF
      request->send(200, "text/plain", "Output Disabled");
    }
    else
    {
      request->send(400, "text/plain", "Invalid output state");
    }
  }
  else
  {
    request->send(400, "text/plain", "Output parameter missing");
  }
}
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
void setup()
{
  WiFiManager wifiManager;                   // Initialize WiFiManager
  wifiManager.autoConnect("Spark Analyzer"); // Auto-connect to WiFi. Change "ESP32_Device" to your desired AP name

  Serial.begin(115200);
  Serial.println("Connected to WiFi");

  initializeUSB_PD();
  for (int i = 0; i < 30; i++)
  {
    processCurrentReading();
  }
  initializeSerialAndPins();

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.print("To access the web app, open your browser and navigate to -> ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html"); });
  server.on("/current", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(current)); });
  server.on("/get_voltage", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(voltage)); });
  server.on("/get_current", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(currentSet)); });
  server.on("/set_voltage", HTTP_GET, handleVoltageChange);

  server.on("/set_output", HTTP_GET, handleOutputControl);
  server.on("/set_current", HTTP_GET, handleCurrentChange);

  server.begin();

  // Debug pin lights up when ready.
  pinMode(debug_led, OUTPUT);
  digitalWrite(debug_led, HIGH);
}

void loop()
{
  updateStatus();
  processCurrentReading();
}

// Initialize Serial and Pin Modes
void initializeSerialAndPins()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  pinMode(usb_pd_int_pin, INPUT);
  pinMode(output_pin, OUTPUT);
  digitalWrite(output_pin, LOW);

  pinMode(current_pin, INPUT);
}

// Initialize USB Power Delivery
void initializeUSB_PD()
{
  Wire.begin(1, 0);
  Wire.setClock(400000);
  PD_UFP.init_PPS(usb_pd_int_pin, PPS_V(5), PPS_A(2.0));
 }

// Update status at intervals
void updateStatus()
{
  if (millis() - lastUpdateTime >= updateInterval)
  {
    lastUpdateTime = millis();
    // Add any periodic update logic here
  }
  PD_UFP.run();
}

// Process current reading and adjust LED status
void processCurrentReading()
{
  if (output)
  {
    digitalWrite(output_pin, HIGH);
  }
  else
  {
    adcError = readFilteredADC(current_pin);
    digitalWrite(output_pin, LOW);
  }
  current = 5.6865 * (readFilteredADC(current_pin) - adcError);
}

// Reads ADC value with a moving average filter
int readFilteredADC(int pin)
{
  int newSample = analogRead(pin);
  adcSum -= adcSamples[adcIndex];
  adcSamples[adcIndex] = newSample;
  adcSum += newSample;
  adcIndex = (adcIndex + 1) % FILTER_LENGTH;
  return adcSum / FILTER_LENGTH;
}
