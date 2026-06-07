#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#endif

#define WIFI_SSID_MAXLEN 32
#define WIFI_PASS_MAXLEN 64
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASS_ADDR 64

class WiFiManager {
public:
  WiFiManager();
  
  // Initialize and start captive portal if needed
  bool begin(unsigned long timeoutMs = 120000);
  
  // Get stored credentials
  String getSSID();
  String getPassword();
  
  // Save credentials to EEPROM
  void saveCredentials(String ssid, String password);
  
  // Clear stored credentials
  void clearCredentials();
  
  // Check if WiFi is connected
  bool isConnected();
  
private:
#ifdef ESP8266
  ESP8266WebServer* server;
#elif defined(ESP32)
  WebServer* server;
#endif
  
  String ssid;
  String password;
  bool credentialsFound;
  
  // Start the access point
  void startAP();
  
  // Start the web server for configuration
  void setupWebServer();
  
  // Handle web requests
  void handleRoot();
  void handleScan();
  void handleSave();
  void handleNotFound();
  
  // Try to connect to WiFi
  bool connectToWiFi(unsigned long timeoutMs);
  
  // Read/write EEPROM
  void loadCredentials();
  void writeStringToEEPROM(int address, String data, int maxLen);
  String readStringFromEEPROM(int address, int maxLen);
};

#endif
