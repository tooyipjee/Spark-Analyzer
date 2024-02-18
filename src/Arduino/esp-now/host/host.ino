#include <esp_now.h>
#include <WiFi.h>

// MAC addresses of the ESP boards
uint8_t broadcastAddress1[] = {0x3C, 0x71, 0xBF, 0xC3, 0xBF, 0xB0};
uint8_t broadcastAddress2[] = {0x24, 0x0A, 0xC4, 0xAE, 0xAE, 0x44};
uint8_t broadcastAddress3[] = {0x80, 0x7D, 0x3A, 0x58, 0xB4, 0xB0};

// Structure to hold voltage (int) and enable (bool)
typedef struct {
  int voltage;
  bool enable;
} voltage_enable_struct;

voltage_enable_struct ve;  // Instance of the structure

esp_now_peer_info_t peerInfo;

// Callback function when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect

  // Display usage guide
  Serial.println("ESP-NOW Usage Guide:");
  Serial.println("Send data in the format: <BoardNumber> <Voltage> <Enable>");
  Serial.println("Where <BoardNumber> is 1, 2, or 3,");
  Serial.println("<Voltage> is an integer (5,9,12,15,20),");
  Serial.println("and <Enable> is 1 (true) or 0 (false).");
  Serial.println("Example: 2 5 1");
  Serial.println();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  
  // register peer
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Registering first peer
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer 1");
    return;
  }

  // Registering second peer
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer 2");
    return;
  }

  // Registering third peer
  memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer 3");
    return;
  }
}

void loop() {
  // Check if serial data is available
  if (Serial.available() > 0) {
    int boardNumber = Serial.parseInt(); // Read board number
    ve.voltage = Serial.parseInt();      // Read voltage
    ve.enable = Serial.parseInt() == 1;  // Read enable (convert to bool)

    uint8_t* targetAddress;
    switch (boardNumber) {
      case 1: targetAddress = broadcastAddress1; break;
      case 2: targetAddress = broadcastAddress2; break;
      case 3: targetAddress = broadcastAddress3; break;
      default: 
        Serial.println("Invalid board number");
        return;
    }

    // Send data to the selected board
    esp_err_t result = esp_now_send(targetAddress, (uint8_t *) &ve, sizeof(voltage_enable_struct));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
  delay(2000);
}
