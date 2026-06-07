/*
  AirDMX Module - Art-Net Node for ESP-01
  
  Receives Art-Net DMX data over WiFi and sends it out via RS485
*/

#include <ArtnetnodeWifi.h>
#include <WiFiManager.h>

// Create the ArtNet node
ArtnetnodeWifi artnet;

// Create WiFi Manager
WiFiManager wifiManager;

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
  
  // Initialize WiFi with captive portal
  // Timeout after 2 minutes (120000ms) of waiting for configuration
  if (!wifiManager.begin(120000)) {
    Serial.println("Failed to connect to WiFi within timeout period");
    // Continue anyway - may reconnect automatically
  }
  
  if (wifiManager.isConnected()) {
    Serial.println("WiFi Connected!");
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
    Serial.println("WiFi not connected. Will attempt to reconnect.");
  }
}

void loop() {
  // Check if WiFi is still connected
  if (!wifiManager.isConnected()) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    WiFi.reconnect();
    delay(5000);
    return;
  }
  
  // Read incoming Art-Net packets
  uint16_t opcode = artnet.read();
  
  if (opcode == OpDmx) {
    // Process DMX data
    handleDMXData();
  }
  
  delay(10);
}
