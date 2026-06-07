/*
  AirDMX Module - Art-Net Node for ESP-01
  
  Receives Art-Net DMX data over WiFi and sends it out via RS485
*/

#include <ArtnetnodeWifi.h>

// Create the ArtNet node
ArtnetnodeWifi artnet;

// WiFi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

void handleDMXData() {
  // Get the DMX frame data
  uint8_t* dmxData = artnet.getDmxFrame();
  
  // Send to RS485 (would be implemented here)
  // For now, just for demonstration
  if (dmxData[0] > 0) {
    // Send data out via Serial/RS485
    Serial.print("DMX Value: ");
    Serial.println(dmxData[0]);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\nStarting AirDMX ArtNet Node...");
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Initialize ArtNet
    artnet.begin("AirDMX");
    artnet.setName("AirDMX Node");
    artnet.setNumPorts(1);
    artnet.setStartingUniverse(0);
    artnet.enableDMXOutput(0);
    artnet.setDMXOutput(0, 0, 0);  // output 0, UART 0, Universe 0
    
    Serial.println("ArtNet Node initialized");
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void loop() {
  // Read incoming Art-Net packets
  uint16_t opcode = artnet.read();
  
  if (opcode == OpDmx) {
    // Process DMX data
    handleDMXData();
  }
  
  delay(10);
}
