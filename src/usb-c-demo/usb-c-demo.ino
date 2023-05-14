#include <Wire.h>
#include "tcpm_driver.h"
#include "usb_pd.h"
#include "OTA.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

const int usb_pd_int_pin = 10;
const int debug_led_pin  = 3;
WiFiManager wm;

// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};
// USB-C Specific - TCPM end 1

void setup() {
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
}

void loop() {  
  ArduinoOTA.handle();
  if (LOW == digitalRead(usb_pd_int_pin)) {
      tcpc_alert(0);
  }

  TelnetStream.println(pd_get_max_voltage());
  TelnetStream.println("looping...");
  pd_run_state_machine(0);
  // For some reason, a delay of 4 ms seems to be best
  // My guess is that spamming the I2C bus too fast causes problems
  delay(4);
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  digitalWrite(debug_led_pin, HIGH);
}

