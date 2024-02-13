#include <Wire.h>
#include "tcpm_driver.h"
#include "usb_pd.h"
#include "OTA.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Preferences.h>

const int usb_pd_int_pin = 10;
const int debug_led_pin  = 3;
const int current_pin = 2;
WiFiManager wm;
Preferences preferences;

// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};
// USB-C Specific - TCPM end 1
int rawVoltage = 0;
bool streamCurrent = false;
bool outputEnable = false;
int voltageSet = 0;
float voltage = 0;
int current = 0;
int adcError = 0;
void setup() {
  preferences.begin("my-app", false); 

  Serial.begin(115200);


  
  // Configure and start the WiFi station
  WiFi.mode(WIFI_STA);

  bool res;
  res = wm.autoConnect("PP_AP", "elektroThing"); // password protected app
  if (!res) {
    Serial.println("Failed to connect");
    wm.setConfigPortalTimeout(60);
    ESP.restart();
  }
  setupOTAWiFiMuted("PowerPod");
  pinMode(usb_pd_int_pin, INPUT);
  pinMode(current_pin, INPUT);

  pinMode(debug_led_pin, OUTPUT);

  digitalWrite(debug_led_pin, HIGH);
  
  Wire.begin(1,0);
  Wire.setClock(400000);
  delay(500);
  digitalWrite(debug_led_pin, LOW);
  tcpm_init(0);
  delay(50);
  pd_init(0);
  delay(50);
  TelnetStream.println(preferences.getInt("Voltage"));
  pd_set_max_voltage(preferences.getInt("Voltage"));

}

void loop() {  
  ArduinoOTA.handle();
  
  voltage = ((analogRead(current_pin)-adcError)*1.0/4096.0)*3.3;
  current = (voltage - 1.65)*10000;
  if(outputEnable==0)
  {
    adcError = analogRead(current_pin)-2048;
  }

  if (LOW == digitalRead(usb_pd_int_pin)) {
      tcpc_alert(0);
      // TelnetStream.println("PD INT PIN TRIGGRED");
  }
  switch (TelnetStream.read()) {
    case '?':
      debug_instructions();
      break;
    case 'V':
      TelnetStream.print("Set Voltage (mV) = ");
      TelnetStream.println(pd_get_max_voltage());
      break;
    case 'I':
      TelnetStream.print("Current Draw = ");
      TelnetStream.println(current);
      break;
    case 'C':
      voltageSet += 1;
      if(voltageSet>=5) {voltageSet = 0;};

      switch(voltageSet)
      {
        case 0:
          TelnetStream.println("Setting to 5V");
          preferences.putInt("Voltage", 5000);
          break;
        case 1:
          TelnetStream.println("Setting to 9V");
          preferences.putInt("Voltage", 9000);
          break;
        case 3:
          TelnetStream.println("Setting to 12V");
          preferences.putInt("Voltage", 12000);
          break;
        case 3:
          TelnetStream.println("Setting to 15V");
          preferences.putInt("Voltage", 15000);
          break;
        case 4:
          TelnetStream.println("Setting to 20V");
          preferences.putInt("Voltage", 20000);
          break;
      }
      break;
    case 'S':
      TelnetStream.println("Toggle Stream");
      delay(300);
      streamCurrent = !streamCurrent;
      break;
    case 'O':
      TelnetStream.println("Toggle Output");
      delay(300);
      outputEnable = !outputEnable;
      break;
    case 'R':
      TelnetStream.println("Restarting now");
      ESP.restart();
  }

  if(streamCurrent)
  {
    TelnetStream.print("Current Draw = ");
    TelnetStream.println(current);
  }

  if(outputEnable)
  {
    digitalWrite(debug_led_pin, HIGH);
  }
  else
  {
    digitalWrite(debug_led_pin, LOW);
  }
  pd_run_state_machine(0);
  // For some reason, a delay of 4 ms seems to be best
  // My guess is that spamming the I2C bus too fast causes problems
  delay(4);
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  digitalWrite(debug_led_pin, LOW);

  TelnetStream.print("Voltage set to ");
  TelnetStream.println(PD_MAX_VOLTAGE_MV);
}

void debug_instructions()
{
  TelnetStream.println("V - Get UCPD voltage");
  TelnetStream.println("I - Get current draw");
  TelnetStream.println("C - Switch set voltage (requires restart)");
  TelnetStream.println("S - Stream current value");
  TelnetStream.println("O - Enable output");
  TelnetStream.println("R - Restart");
}