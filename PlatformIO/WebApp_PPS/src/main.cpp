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

void initializeSerial();
void initializePins();
void initializeUSB_PD();
void printStatus();
void updateStatus();
void processCurrentReading();
int readFilteredADC(int pin);

// User-configurable constants
#define FILTER_LENGTH                      10
#define DEFAULT_PPS_OUTPUT_STATE           false // true for On, false for Off
#define DEFAULT_PPS_OUTPUT_VOLTAGE_V       5
#define DEFAULT_PPS_OUTPUT_CURRENT_LIMIT_A 1
#define UART                               Serial // debug output via the USB C port
// #define UART                            Serial0 // debut output via the UART pin header
const unsigned long updateIntervalMs =     10000; // Set debug output rate

// Filter variables
int adcSamples[FILTER_LENGTH];
int adcIndex = 0;
int adcSum = 0;

unsigned long lastUpdateTime = 0;
const int usb_pd_int_pin = 10;
const int output_pin = 3;
const int current_pin = 2;
const int debug_led = 8;
int measuredCurrentMA = 0;
bool ppsOutputEnabled = DEFAULT_PPS_OUTPUT_STATE;
float ppsOutputVoltageV = DEFAULT_PPS_OUTPUT_VOLTAGE_V;
float ppsOutputCurrentLimitA = DEFAULT_PPS_OUTPUT_CURRENT_LIMIT_A;
int adcError = 0;

PD_UFP_c PD_UFP;


void handlePpsOutputCurrentLimitAChange(AsyncWebServerRequest *request) {
  if (request->hasParam("ppsOutputCurrentLimitA")) {
    float newCurrentA = request->getParam("ppsOutputCurrentLimitA")->value().toFloat();
    if (newCurrentA != ppsOutputCurrentLimitA) {
      ppsOutputCurrentLimitA = newCurrentA;
      UART.printf("PPS Current limit changed to %5.3f A\n", ppsOutputCurrentLimitA);
      bool retval = PD_UFP.set_PPS(PPS_V(ppsOutputVoltageV), PPS_A(ppsOutputCurrentLimitA));
      UART.printf("set_PPS(): %u\n", retval);
      request->send(200, "text/plain", "PPS Current limit updated");
    } else {
      request->send(200, "text/plain", "PPS Current limit unchanged");
    }
  } else {
    request->send(400, "text/plain", "PPS Current parameter missing");
  }
}

void handlePpsOutputVoltageVChange(AsyncWebServerRequest *request)
{
  if (request->hasParam("ppsOutputVoltageV"))
  {
    float newVoltageV = request->getParam("ppsOutputVoltageV")->value().toFloat();
    if (newVoltageV != ppsOutputVoltageV)
    {
      ppsOutputVoltageV = newVoltageV;
      UART.printf("PPS Voltage changed to %5.3f V\n", ppsOutputVoltageV);
      // esp_restart();                  // Restart ESP32-C3 to apply new voltage setting
      bool retval = PD_UFP.set_PPS(PPS_V(ppsOutputVoltageV), PPS_A(ppsOutputCurrentLimitA));
      UART.printf("set_PPS(): %u\n", retval);
      request->send(200, "text/plain", "PPS Voltage updated");
    }
    else
    {
      UART.printf("PPS Voltage unchanged\n");
      request->send(200, "text/plain", "PPS Voltage unchanged");
    }
  }
  else
  {
    UART.printf("PPS Voltage parameter missing\n");
    request->send(400, "text/plain", "PPS Voltage parameter missing");
  }
}

void handlePpsOutputState(AsyncWebServerRequest *request)
{
  if (request->hasParam("ppsOutputState"))
  {
    String newOutputState = request->getParam("ppsOutputState")->value();
    if (newOutputState == "1")
    {
      ppsOutputEnabled = true;
      digitalWrite(output_pin, HIGH); // Turn output ON
      request->send(200, "text/plain", "PPS Output enabled");
    }
    else if (newOutputState == "0")
    {
      ppsOutputEnabled = false;
      digitalWrite(output_pin, LOW); // Turn output OFF
      request->send(200, "text/plain", "PPS Output disabled");
    }
    else
    {
      request->send(400, "text/plain", "PPS Output parameter invalid");
    }
  }
  else
  {
    request->send(400, "text/plain", "PPS Output parameter missing");
  }
}

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup()
{
  initializeSerial();

  WiFiManager wifiManager;                   // Initialize WiFiManager
  wifiManager.autoConnect("Spark Analyzer"); // Auto-connect to WiFi. Change "ESP32_Device" to your desired AP name
  UART.printf("Connected to WiFi: %s\n", wifiManager.getWiFiSSID().c_str());

  initializeUSB_PD();
  for (int i = 0; i < 30; i++)
  {
    processCurrentReading();
  }
  initializePins();
  printStatus();

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    UART.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  UART.println("To access the web app, open your browser and navigate to -> ");
  UART.printf("http://%s\n", WiFi.localIP().toString().c_str());

  server.on("/",                               HTTP_GET, [](AsyncWebServerRequest *request)  { request->send(SPIFFS, "/index.html"); });
  server.on("/get_measured_current_mA",        HTTP_GET, [](AsyncWebServerRequest *request)  { request->send(200, "text/plain", String(measuredCurrentMA)); });
  server.on("/get_pps_output_voltage_V",       HTTP_GET, [](AsyncWebServerRequest *request)  { request->send(200, "text/plain", String(ppsOutputVoltageV)); });
  server.on("/get_pps_output_current_limit_A", HTTP_GET, [](AsyncWebServerRequest *request)  { request->send(200, "text/plain", String(ppsOutputCurrentLimitA)); });
  server.on("/set_pps_output_state",           HTTP_GET, handlePpsOutputState);
  server.on("/set_pps_output_voltage_V",       HTTP_GET, handlePpsOutputVoltageVChange);
  server.on("/set_pps_output_current_limit_A", HTTP_GET, handlePpsOutputCurrentLimitAChange);
  server.begin();

  printStatus();
  // Debug pin lights up when ready.
  pinMode(debug_led, OUTPUT);
  digitalWrite(debug_led, HIGH);
  UART.println("Setup complete");
}

void loop()
{
  updateStatus();
  processCurrentReading();
}

// Initialize serial ports
void initializeSerial()
{
  // initialize potentially *both* uarts
  // 1. all the debugging from the esp code and from libraries go to the default port "Serial"
  // 2. the debugging output from Spark Analyzer code can be redirected to the other port "Serial0" based on the definition of the UART macro
  Serial.begin(115200);
  UART.begin(115200);
  UART.println("Initialized serial");
}

// Initialize pin modes
void initializePins()
{
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

// Print system status
void printStatus()
{
  UART.printf("[%10u] PPS mode, Voltage: %5.3f V, Current: %d mA (%5.3f A), Output: %s\n",
    millis(),
    ppsOutputVoltageV,
    measuredCurrentMA,
    ppsOutputCurrentLimitA,
    ppsOutputEnabled ? "enabled" : "disabled");
}

// Update status at intervals
void updateStatus()
{
  if (millis() - lastUpdateTime >= updateIntervalMs)
  {
    lastUpdateTime = millis();

    // Add any periodic update logic below
    printStatus();
  }
  PD_UFP.run();
}

// Process current reading and adjust LED status
void processCurrentReading()
{
  if (ppsOutputEnabled)
  {
    digitalWrite(output_pin, HIGH);
  }
  else
  {
    adcError = readFilteredADC(current_pin);
    digitalWrite(output_pin, LOW);
  }
  measuredCurrentMA = 5.6865 * (readFilteredADC(current_pin) - adcError);
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
