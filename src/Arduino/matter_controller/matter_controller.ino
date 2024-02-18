#include <Arduino.h>
#include <Wire.h>
#include "tcpm_driver.h"
#include "usb_pd.h"
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;


// User-configurable constants
#define VOLTAGE 12

/**
 * This program presents example Matter light device with OnOff cluster by
 * controlling LED with Matter and toggle button.
 *
 * If your ESP does not have buildin LED, please connect it to LED_PIN
 *
 * You can toggle light by both:
 *  - Matter (via CHIPTool or other Matter controller)
 *  - toggle button (by default attached to GPIO0 - reset button, with debouncing) 
 */

// Please configure your PINs
const int LED_PIN = 3;
const int TOGGLE_BUTTON_PIN = 0;

// Debounce for toggle button
const int DEBOUNCE_DELAY = 500;
int last_toggle;

// Cluster and attribute ID used by Matter light device
const uint32_t CLUSTER_ID = OnOff::Id;
const uint32_t ATTRIBUTE_ID = OnOff::Attributes::OnOff::Id;

// Endpoint and attribute ref that will be assigned to Matter device
uint16_t light_endpoint_id = 0;
attribute_t *attribute_ref;

// There is possibility to listen for various device events, related for example to setup process
// Leaved as empty for simplicity
static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}
static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id,
                                   uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
  return ESP_OK;
}

// Listener on attribute update requests.
// In this example, when update is requested, path (endpoint, cluster and attribute) is checked
// if it matches light attribute. If yes, LED changes state to new one.
static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
  if (type == attribute::PRE_UPDATE && endpoint_id == light_endpoint_id &&
      cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID) {
    // We got an light on/off attribute update!
    bool new_state = val->val.b;
    digitalWrite(LED_PIN, new_state);
  }
  return ESP_OK;
}
const int usb_pd_int_pin = 10;
int voltage = VOLTAGE;

// USB-C Specific - TCPM start
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};

void setup() {
  initializeSerialAndPins();
  initializeUSB_PD();

  // Enable debug logging
  esp_log_level_set("*", ESP_LOG_DEBUG);

  // Setup Matter node
  node::config_t node_config;
  node_t *node = node::create(&node_config, on_attribute_update, on_identification);

  // Setup Light endpoint / cluster / attributes with default values
  on_off_light::config_t light_config;
  light_config.on_off.on_off = false;
  light_config.on_off.lighting.start_up_on_off = false;
  endpoint_t *endpoint = on_off_light::create(node, &light_config, ENDPOINT_FLAG_NONE, NULL);

  // Save on/off attribute reference. It will be used to read attribute value later.
  attribute_ref = attribute::get(cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);

  // Save generated endpoint id
  light_endpoint_id = endpoint::get_id(endpoint);
  
  // Setup DAC (this is good place to also set custom commission data, passcodes etc.)
  esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

  // Start Matter device
  esp_matter::start(on_device_event);

  // Print codes needed to setup Matter device
  PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

}

// Reads light on/off attribute value
esp_matter_attr_val_t get_onoff_attribute_value() {
  esp_matter_attr_val_t onoff_value = esp_matter_invalid(NULL);
  attribute::get_val(attribute_ref, &onoff_value);
  return onoff_value;
}

// Sets light on/off attribute value
void set_onoff_attribute_value(esp_matter_attr_val_t* onoff_value) {
  attribute::update(light_endpoint_id, CLUSTER_ID, ATTRIBUTE_ID, onoff_value);
}

void loop() {
  updateStatus();
   if ((millis() - last_toggle) > DEBOUNCE_DELAY) {
    if (!digitalRead(TOGGLE_BUTTON_PIN)) {
      last_toggle = millis();
      // Read actual on/off value, invert it and set
      esp_matter_attr_val_t onoff_value = get_onoff_attribute_value();
      onoff_value.val.b = !onoff_value.val.b;
      set_onoff_attribute_value(&onoff_value);
    }
  }
}



// Initialize Serial and Pin Modes
void initializeSerialAndPins() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  pinMode(usb_pd_int_pin, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
}

// Initialize USB Power Delivery
void initializeUSB_PD() {
  Wire.begin(1, 0);
  Wire.setClock(400000);
  delay(500);
  tcpm_init(0);
  delay(50);
  pd_init(0);
  delay(50);
  pd_set_max_voltage(voltage*1000); // Set default 5V
}

// Update status at intervals
void updateStatus() {

  pd_run_state_machine(0);
}
