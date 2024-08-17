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
#include <Preferences.h>

void initializeSerial();
void initializePins();
void initializeWifi();
void initializeUSB_PD();
void printStatus();
void updateStatus();
void processCurrentReading();
int readFilteredADC(int pin);

// User-configurable constants
#define USE_WIFI_MANAGER                   true // true for using WifiManger, false for using hardcoded wifi credentials - see FILL-ME-IN below
#define FILTER_LENGTH                      10
#define DEFAULT_PPS_OUTPUT_STATE           false // true for On, false for Off
#define DEFAULT_PPS_OUTPUT_VOLTAGE_V       5
#define DEFAULT_PPS_OUTPUT_CURRENT_LIMIT_A 1
#define DEBUG_VIA_DEFAULT_UART             true // true for default uart (USB), false for secondary uart (GPIO pins)
const unsigned long updateIntervalMs =     10000; // Set debug output rate

// Filter variables
int adcSamples[FILTER_LENGTH];
int adcIndex = 0;
int adcSum = 0;

#if DEBUG_VIA_DEFAULT_UART
#define UART Serial
#else
#define UART Serial0
#endif

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

Preferences preferences;
// Keys used for Preferences storage
// N.B. keys may not be longer than 15 characters
#define PREFERENCE_PPS_OUTPUT_VOLTAGE_V "ppsVoltageV"
#define PREFERENCE_PPS_OUTPUT_CURRENT_A "ppsCurrentA"

void handlePpsOutputCurrentLimitAChange(AsyncWebServerRequest *request) {
  if (request->hasParam("ppsOutputCurrentLimitA")) {
    float newCurrentA = request->getParam("ppsOutputCurrentLimitA")->value().toFloat();
    if (newCurrentA != ppsOutputCurrentLimitA) {
      ppsOutputCurrentLimitA = newCurrentA;
      UART.printf("PPS Current limit changed to %5.3f A\n", ppsOutputCurrentLimitA);
      bool retval = PD_UFP.set_PPS(PPS_V(ppsOutputVoltageV), PPS_A(ppsOutputCurrentLimitA));
      UART.printf("set_PPS(): %u\n", retval);
      preferences.putFloat(PREFERENCE_PPS_OUTPUT_CURRENT_A, ppsOutputCurrentLimitA);
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
      preferences.putFloat(PREFERENCE_PPS_OUTPUT_VOLTAGE_V, ppsOutputVoltageV);
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
      UART.printf("PPS Output enabled\n");
      request->send(200, "text/plain", "PPS Output enabled");
    }
    else if (newOutputState == "0")
    {
      ppsOutputEnabled = false;
      digitalWrite(output_pin, LOW); // Turn output OFF
      UART.printf("PPS Output disabled\n");
      request->send(200, "text/plain", "PPS Output disabled");
    }
    else
    {
      UART.printf("PPS Output parameter invalid\n");
      request->send(400, "text/plain", "PPS Output parameter invalid");
    }
  }
  else
  {
    UART.printf("PPS Output parameter missing\n");
    request->send(400, "text/plain", "PPS Output parameter missing");
  }
}

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup()
{
  initializeSerial();

  // Initialize Preferences with namespace "storage". False for read/write mode.
  preferences.begin("storage", false);
  UART.printf("preferences.getType(V): %u\n", preferences.getType(PREFERENCE_PPS_OUTPUT_VOLTAGE_V));
  UART.printf("preferences.getType(A): %u\n", preferences.getType(PREFERENCE_PPS_OUTPUT_CURRENT_A));
  // Retrieve stored values or use defaults if not set.
  ppsOutputVoltageV =      preferences.getFloat(PREFERENCE_PPS_OUTPUT_VOLTAGE_V, DEFAULT_PPS_OUTPUT_VOLTAGE_V);
  ppsOutputCurrentLimitA = preferences.getFloat(PREFERENCE_PPS_OUTPUT_CURRENT_A, DEFAULT_PPS_OUTPUT_CURRENT_LIMIT_A);

  UART.printf("Mode: PPS\n");
  UART.printf("Voltage: %5.3f V\n", ppsOutputVoltageV);
  UART.printf("Current limit: %5.3f A\n", ppsOutputCurrentLimitA);

  initializeWifi();

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
  Serial.println("Setup complete");
  Serial0.println("Setup complete");
}

void loop()
{
  updateStatus();
  processCurrentReading();
}

// Initialize serial ports
void initializeSerial()
{
  Serial.begin(115200);
  Serial.println("Initializing default serial port (USB connector)");

  Serial0.begin(115200);
  Serial0.println("Initializing secondary serial port (GPIO pins)");

  Serial.println("Debug messages from most libraries will be printed on this serial port");

#if DEBUG_VIA_DEFAULT_UART == true
  Serial.println("Debug messages from Spark Analyzer code will be printed on this serial port");
  Serial0.println("Debug messages from Spark Analyzer code will be printed on the other serial port");
#else
  Serial0.println("Debug messages from Spark Analyzer code will be printed on this serial port");
  Serial.println("Debug messages from Spark Analyzer code will be printed on the other serial port");
#endif
}

// Initialize pin modes
void initializePins()
{
  pinMode(usb_pd_int_pin, INPUT);

  pinMode(output_pin, OUTPUT);
  digitalWrite(output_pin, LOW);

  pinMode(current_pin, INPUT);
}

void initializeWifi()
{
#if USE_WIFI_MANAGER
  UART.println("Connecting to wifi with WiFiManager");
  WiFiManager wifiManager(UART);
  wifiManager.autoConnect("Spark Analyzer"); // Auto-connect to previously stored WiFi, or fall back to creating a "Spark Analyzer" access point for configuration
#else
  // Change ssid and pwd as needed
  String ssid = "FILL-ME-IN";
  String pwd = "FILL-ME-IN";
  UART.printf("Connecting to wifi %s ", ssid.c_str());

  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      UART.print(".");
  }
  UART.println();
#endif

  Serial.printf("Connected to wifi: %s (%d)\nTo access the web app, open your browser and navigate to -> http://%s\n", WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.localIP().toString().c_str());
  Serial0.printf("Connected to wfif: %s (%d)\nTo access the web app, open your browser and navigate to -> http://%s\n", WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.localIP().toString().c_str());
}

// Initialize USB Power Delivery
void initializeUSB_PD()
{
  Wire.begin(1, 0);
  Wire.setClock(400000);
  PD_UFP.init_PPS(usb_pd_int_pin, PPS_V(ppsOutputVoltageV), PPS_A(ppsOutputCurrentLimitA));
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
