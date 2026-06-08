/*
  AirDMX Module - Art-Net Node for ESP-01
  
  Receives Art-Net DMX data over WiFi and sends it out via RS485
*/

#include <ArtnetnodeWifi.h>
#include <WiFiManager.h>
#define DMX_LED_PIN 0   // GPIO0, active LOW

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
  // Initialize ArtNet
  artnet.begin("AirDMX");
  artnet.setName("AirDMX Node");
  artnet.setNumPorts(1);
  artnet.setStartingUniverse(0);
  artnet.enableDMXOutput(0);
  artnet.setDMXOutput(0, 0, 0);  // output 0, UART 0, Universe 0
    

  
pinMode(DMX_LED_PIN, OUTPUT);
digitalWrite(DMX_LED_PIN, HIGH);  // LED off (active low)
}

void loop() {
  // Check if WiFi is still connected
  if (!wifiManager.isConnected()) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastReconnectAttempt > 5000) {
      WiFi.reconnect();
      lastReconnectAttempt = currentTime;
    }
    delay(100);
    return;
  }
  
  // Read incoming Art-Net packets
  uint16_t opcode = artnet.read();
  
  if (opcode == OpDmx) {
    // Flash LED on DMX send
    digitalWrite(DMX_LED_PIN, LOW);   // LED ON
    // Process DMX data
    handleDMXData();
    digitalWrite(DMX_LED_PIN, HIGH);  // LED OFF
  }
  
  delay(10);
}
