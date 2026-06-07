#include "WiFiManager.h"

WiFiManager::WiFiManager() : server(nullptr), credentialsFound(false) {
  EEPROM.begin(EEPROM_SIZE);
  loadCredentials();
}

bool WiFiManager::begin(unsigned long timeoutMs) {
  Serial.println("\n[WiFiManager] Initializing...");
  
  // Try to connect with stored credentials if available
  if (credentialsFound && ssid.length() > 0) {
    Serial.print("[WiFiManager] Attempting to connect to: ");
    Serial.println(ssid);
    
    if (connectToWiFi(timeoutMs)) {
      Serial.println("[WiFiManager] Successfully connected to WiFi!");
      Serial.print("[WiFiManager] IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    Serial.println("[WiFiManager] Failed to connect with stored credentials");
  } else {
    Serial.println("[WiFiManager] No stored credentials found");
  }
  
  // Start captive portal
  Serial.println("[WiFiManager] Starting access point for configuration...");
  startAP();
  setupWebServer();
  
  // Wait for user to configure WiFi
  unsigned long startTime = millis();
  while (!isConnected() && (millis() - startTime) < timeoutMs) {
    if (server) {
      server->handleClient();
    }
    delay(10);
  }
  
  // Clean up server
  if (server) {
    server->stop();
    delete server;
    server = nullptr;
  }
  
  WiFi.mode(WIFI_STA);
  return isConnected();
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getSSID() {
  return ssid;
}

String WiFiManager::getPassword() {
  return password;
}

void WiFiManager::saveCredentials(String newSSID, String newPassword) {
  ssid = newSSID;
  password = newPassword;
  
  writeStringToEEPROM(SSID_ADDR, ssid, WIFI_SSID_MAXLEN);
  writeStringToEEPROM(PASS_ADDR, password, WIFI_PASS_MAXLEN);
  EEPROM.commit();
  
  Serial.println("[WiFiManager] Credentials saved to EEPROM");
}

void WiFiManager::clearCredentials() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  ssid = "";
  password = "";
  credentialsFound = false;
  Serial.println("[WiFiManager] Credentials cleared");
}

void WiFiManager::startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("AirDMX-Setup", "airdmx123");
  
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  Serial.println("[WiFiManager] Access Point started");
  Serial.println("[WiFiManager] SSID: AirDMX-Setup");
  Serial.println("[WiFiManager] Password: airdmx123");
  Serial.println("[WiFiManager] IP: 192.168.4.1");
}

void WiFiManager::setupWebServer() {
#ifdef ESP8266
  server = new ESP8266WebServer(80);
#elif defined(ESP32)
  server = new WebServer(80);
#endif
  
  server->on("/", [this]() { handleRoot(); });
  server->on("/scan", [this]() { handleScan(); });
  server->on("/save", [this]() { handleSave(); });
  server->onNotFound([this]() { handleNotFound(); });
  
  server->begin();
  Serial.println("[WiFiManager] Web server started on port 80");
}

void WiFiManager::handleRoot() {
  String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <title>AirDMX WiFi Setup</title>
      <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { background: white; padding: 20px; border-radius: 5px; max-width: 400px; margin: 0 auto; }
        h1 { color: #333; }
        input, select { width: 100%; padding: 10px; margin: 10px 0; box-sizing: border-box; }
        button { width: 100%; padding: 10px; background: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
        button:hover { background: #45a049; }
        #networks { max-height: 200px; overflow-y: auto; }
        .network { padding: 8px; margin: 5px 0; background: #f9f9f9; border: 1px solid #ddd; border-radius: 3px; cursor: pointer; }
        .network:hover { background: #f0f0f0; }
      </style>
    </head>
    <body>
      <div class="container">
        <h1>AirDMX WiFi Configuration</h1>
        <form method="POST" action="/save">
          <label for="ssid">WiFi Network:</label>
          <div id="networks">Loading networks...</div>
          <input type="text" id="ssid" name="ssid" placeholder="Or enter SSID manually" required>
          
          <label for="password">Password:</label>
          <input type="password" id="password" name="password" placeholder="WiFi password" required>
          
          <button type="submit">Connect</button>
        </form>
      </div>
      
      <script>
        function loadNetworks() {
          fetch('/scan')
            .then(r => r.json())
            .then(data => {
              const networks = document.getElementById('networks');
              networks.innerHTML = '';
              data.forEach(network => {
                const div = document.createElement('div');
                div.className = 'network';
                div.textContent = network.ssid + ' (Signal: ' + network.rssi + ' dBm)';
                div.onclick = () => {
                  document.getElementById('ssid').value = network.ssid;
                  document.getElementById('password').focus();
                };
                networks.appendChild(div);
              });
            });
        }
        loadNetworks();
      </script>
    </body>
    </html>
  )";
  
  server->send(200, "text/html", html);
}

void WiFiManager::handleScan() {
  int networksFound = WiFi.scanNetworks();
  
  String json = "[";
  for (int i = 0; i < networksFound; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  
  server->send(200, "application/json", json);
}

void WiFiManager::handleSave() {
  if (!server->hasArg("ssid") || !server->hasArg("password")) {
    server->send(400, "text/plain", "Missing SSID or password");
    return;
  }
  
  String newSSID = server->arg("ssid");
  String newPassword = server->arg("password");
  
  saveCredentials(newSSID, newPassword);
  
  String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Connecting...</title>
      <style>
        body { font-family: Arial; text-align: center; margin-top: 50px; }
        .spinner { border: 4px solid #f3f3f3; border-top: 4px solid #3498db; border-radius: 50%; width: 40px; height: 40px; animation: spin 1s linear infinite; margin: 20px auto; }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
      </style>
    </head>
    <body>
      <h1>Connecting to WiFi...</h1>
      <p>Attempting to connect to: <strong>)" + newSSID + R"(</strong></p>
      <div class="spinner"></div>
      <p>This page will close when the connection is established.</p>
    </body>
    </html>
  )";
  
  server->send(200, "text/html", html);
  
  // Connect to WiFi
  delay(1000);
  connectToWiFi(30000);
}

void WiFiManager::handleNotFound() {
  server->send(404, "text/plain", "Not Found");
}

bool WiFiManager::connectToWiFi(unsigned long timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  Serial.print("[WiFiManager] Connecting to WiFi");
  unsigned long startTime = millis();
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeoutMs) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    if (server) {
      server->handleClient();
    }
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFiManager] WiFi connected!");
    return true;
  } else {
    Serial.println("[WiFiManager] WiFi connection failed");
    return false;
  }
}

void WiFiManager::loadCredentials() {
  ssid = readStringFromEEPROM(SSID_ADDR, WIFI_SSID_MAXLEN);
  password = readStringFromEEPROM(PASS_ADDR, WIFI_PASS_MAXLEN);
  
  credentialsFound = (ssid.length() > 0 && password.length() > 0);
  
  if (credentialsFound) {
    Serial.print("[WiFiManager] Found stored SSID: ");
    Serial.println(ssid);
  }
}

void WiFiManager::writeStringToEEPROM(int address, String data, int maxLen) {
  for (int i = 0; i < maxLen; i++) {
    if (i < data.length()) {
      EEPROM.write(address + i, data[i]);
    } else {
      EEPROM.write(address + i, 0);
    }
  }
}

String WiFiManager::readStringFromEEPROM(int address, int maxLen) {
  String result = "";
  for (int i = 0; i < maxLen; i++) {
    char c = EEPROM.read(address + i);
    if (c == 0) break;
    result += c;
  }
  return result;
}