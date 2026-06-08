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
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("   Starting AirDMX ArtNet Node");
  Serial.println("========================================");
  Serial.println();
  
  // Print board info
  Serial.println("[MAIN] Board Information:");
#ifdef ESP8266
  Serial.println("[MAIN] Platform: ESP8266");
  Serial.print("[MAIN] Chip ID: ");
  Serial.println(ESP.getChipId(), HEX);
  Serial.print("[MAIN] Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
#elif defined(ESP32)
  Serial.println("[MAIN] Platform: ESP32");
  Serial.print("[MAIN] Chip ID: ");
  Serial.println((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
  Serial.print("[MAIN] Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
#else
  Serial.println("[MAIN] Platform: Unknown");
#endif
  Serial.println();
  
  Serial.println("[MAIN] Initializing WiFi Manager...");
  
  // Initialize WiFi with captive portal
  // Timeout after 2 minutes (120000ms) of waiting for configuration
  Serial.println("[MAIN] Calling wifiManager.begin() with 120000ms timeout...");
  bool wifiResult = wifiManager.begin(120000);
  
  Serial.print("[MAIN] wifiManager.begin() returned: ");
  Serial.println(wifiResult ? "true" : "false");
  Serial.println();
  
  if (wifiResult) {
    Serial.println("[MAIN] WiFi initialization succeeded!");
  } else {
    Serial.println("[MAIN] WiFi initialization failed or timed out");
  }
  
  if (wifiManager.isConnected()) {
    Serial.println("[MAIN] WiFi Connected!");
    Serial.print("[MAIN] IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[MAIN] Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    // Initialize ArtNet
    Serial.println("[MAIN] Initializing ArtNet node...");
    artnet.begin("AirDMX");
    artnet.setName("AirDMX Node");
    artnet.setNumPorts(1);
    artnet.setStartingUniverse(0);
    artnet.enableDMXOutput(0);
    artnet.setDMXOutput(0, 0, 0);  // output 0, UART 0, Universe 0
    
    Serial.println("[MAIN] ArtNet Node initialized");
  } else {
    Serial.println("[MAIN] WiFi not connected. Will attempt to reconnect in loop.");
    Serial.println("[MAIN] Current WiFi status: ");
    Serial.print("[MAIN]   Status code: ");
    Serial.println(WiFi.status());
  }
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("   Setup Complete - Entering Main Loop");
  Serial.println("========================================");
  Serial.println();

pinMode(DMX_LED_PIN, OUTPUT);
digitalWrite(DMX_LED_PIN, HIGH);  // LED off (active low)
}

void loop() {
  // Check if WiFi is still connected
  if (!wifiManager.isConnected()) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastReconnectAttempt > 5000) {
      Serial.println("[LOOP] WiFi disconnected, attempting to reconnect...");
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
