#include "WiFiManager.h"

WiFiManager::WiFiManager() : server(nullptr), credentialsFound(false) {
  Serial.println("[WiFiManager] Constructor called");
  EEPROM.begin(EEPROM_SIZE);
  loadCredentials();
  Serial.println("[WiFiManager] Constructor complete");
}

bool WiFiManager::begin(unsigned long timeoutMs) {
  Serial.println("\n[WiFiManager] ===== BEGIN CALLED =====");
  Serial.print("[WiFiManager] Timeout set to: ");
  Serial.print(timeoutMs);
  Serial.println(" ms");
  
  Serial.println("[WiFiManager] Initializing...");
  
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
    Serial.print("[WiFiManager] credentialsFound: ");
    Serial.println(credentialsFound);
    Serial.print("[WiFiManager] ssid length: ");
    Serial.println(ssid.length());
  }
  
  // Start captive portal
  Serial.println("[WiFiManager] Starting access point for configuration...");
  startAP();
  
  Serial.println("[WiFiManager] Setting up web server...");
  setupWebServer();
  
  Serial.println("[WiFiManager] Access point is ready for connections.");
  Serial.println("[WiFiManager] Waiting for user to configure WiFi...");
  Serial.print("[WiFiManager] Waiting for up to ");
  Serial.print(timeoutMs / 1000);
  Serial.println(" seconds...");
  
  // Wait for user to configure WiFi
  unsigned long startTime = millis();
  unsigned long lastLogTime = startTime;
  int logIntervalMs = 5000; // Log every 5 seconds
  
  while (!isConnected() && (millis() - startTime) < timeoutMs) {
    if (server) {
      server->handleClient();
    }
    
    // Log progress every 5 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastLogTime >= logIntervalMs) {
      Serial.print("[WiFiManager] Still waiting... ");
      Serial.print((currentTime - startTime) / 1000);
      Serial.println(" seconds elapsed");
      lastLogTime = currentTime;
    }
    
    delay(10);
  }
  
  Serial.println("[WiFiManager] Exited configuration loop");
  unsigned long elapsedTime = millis() - startTime;
  Serial.print("[WiFiManager] Total time in configuration: ");
  Serial.print(elapsedTime);
  Serial.println(" ms");
  
  // Clean up server
  if (server) {
    Serial.println("[WiFiManager] Stopping web server...");
    server->stop();
    delete server;
    server = nullptr;
    Serial.println("[WiFiManager] Web server stopped");
  }
  
  Serial.println("[WiFiManager] Switching to STA mode...");
  WiFi.mode(WIFI_STA);
  
  bool connected = isConnected();
  Serial.print("[WiFiManager] Final connection status: ");
  Serial.println(connected ? "CONNECTED" : "NOT CONNECTED");
  
  Serial.println("[WiFiManager] ===== BEGIN COMPLETE =====");
  return connected;
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
  Serial.println("\n[WiFiManager] === SAVING CREDENTIALS ===");
  Serial.print("[WiFiManager] SSID: ");
  Serial.println(newSSID);
  Serial.print("[WiFiManager] Password length: ");
  Serial.println(newPassword.length());
  
  ssid = newSSID;
  password = newPassword;
  
  writeStringToEEPROM(SSID_ADDR, ssid, WIFI_SSID_MAXLEN);
  writeStringToEEPROM(PASS_ADDR, password, WIFI_PASS_MAXLEN);
  EEPROM.commit();
  
  Serial.println("[WiFiManager] Credentials saved to EEPROM");
}

void WiFiManager::clearCredentials() {
  Serial.println("[WiFiManager] Clearing all EEPROM...");
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
  Serial.println("[WiFiManager] Setting WiFi mode to AP...");
  WiFi.mode(WIFI_AP);
  delay(100);
  
  Serial.println("[WiFiManager] Creating soft AP...");
  bool apStarted = WiFi.softAP("AirDMX-Setup", "airdmx123");
  Serial.print("[WiFiManager] Soft AP started: ");
  Serial.println(apStarted ? "SUCCESS" : "FAILED");
  
  delay(100);
  
  Serial.println("[WiFiManager] Configuring AP IP...");
  IPAddress apIP(192, 168, 4, 1);
  bool ipConfigured = WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  Serial.print("[WiFiManager] AP IP configured: ");
  Serial.println(ipConfigured ? "SUCCESS" : "FAILED");
  
  Serial.println("\n========== ACCESS POINT INFO ==========");
  Serial.println("[WiFiManager] Access Point started");
  Serial.println("[WiFiManager] SSID: AirDMX-Setup");
  Serial.println("[WiFiManager] Password: airdmx123");
  Serial.println("[WiFiManager] IP Address: 192.168.4.1");
  Serial.println("========================================\n");
  
  IPAddress apAddress = WiFi.softAPIP();
  Serial.print("[WiFiManager] Actual AP IP: ");
  Serial.println(apAddress);
}

void WiFiManager::setupWebServer() {
  Serial.println("[WiFiManager] Creating web server instance...");
  
#ifdef ESP8266
  server = new ESP8266WebServer(80);
  Serial.println("[WiFiManager] Using ESP8266WebServer");
#elif defined(ESP32)
  server = new WebServer(80);
  Serial.println("[WiFiManager] Using ESP32 WebServer");
#else
  Serial.println("[WiFiManager] WARNING: Unknown board type!");
#endif
  
  if (!server) {
    Serial.println("[WiFiManager] ERROR: Failed to allocate server!");
    return;
  }
  
  Serial.println("[WiFiManager] Setting up route handlers...");
  server->on("/", [this]() { handleRoot(); });
  server->on("/scan", [this]() { handleScan(); });
  server->on("/save", [this]() { handleSave(); });
  server->onNotFound([this]() { handleNotFound(); });
  
  Serial.println("[WiFiManager] Starting web server...");
  server->begin();
  Serial.println("[WiFiManager] Web server started on port 80");
}

void WiFiManager::handleRoot() {
  Serial.println("[WiFiManager] Handling / request");
  
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
  Serial.println("[WiFiManager] Handling /scan request");
  Serial.println("[WiFiManager] Starting WiFi scan...");
  
  int networksFound = WiFi.scanNetworks();
  
  Serial.print("[WiFiManager] Networks found: ");
  Serial.println(networksFound);
  
  String json = "[";
  for (int i = 0; i < networksFound; i++) {
    if (i > 0) json += ",";
    String ssidName = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    json += "{\"ssid\":\"" + ssidName + "\",\"rssi\":" + String(rssi) + "}";
    
    Serial.print("[WiFiManager]   Network ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(ssidName);
    Serial.print(" (RSSI: ");
    Serial.print(rssi);
    Serial.println(")");
  }
  json += "]";
  
  server->send(200, "application/json", json);
}

void WiFiManager::handleSave() {
  Serial.println("\n[WiFiManager] Handling /save request");
  
  if (!server->hasArg("ssid") || !server->hasArg("password")) {
    Serial.println("[WiFiManager] ERROR: Missing SSID or password in request");
    server->send(400, "text/plain", "Missing SSID or password");
    return;
  }
  
  String newSSID = server->arg("ssid");
  String newPassword = server->arg("password");
  
  Serial.print("[WiFiManager] Received SSID: ");
  Serial.println(newSSID);
  Serial.print("[WiFiManager] Received password length: ");
  Serial.println(newPassword.length());
  
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
  Serial.println("[WiFiManager] Attempting WiFi connection...");
  delay(1000);
  connectToWiFi(30000);
}

void WiFiManager::handleNotFound() {
  Serial.print("[WiFiManager] 404 Not Found: ");
  Serial.println(server->uri());
  server->send(404, "text/plain", "Not Found");
}

bool WiFiManager::connectToWiFi(unsigned long timeoutMs) {
  Serial.println("\n[WiFiManager] === CONNECTING TO WIFI ===");
  Serial.print("[WiFiManager] SSID: ");
  Serial.println(ssid);
  Serial.print("[WiFiManager] Password length: ");
  Serial.println(password.length());
  Serial.print("[WiFiManager] Timeout: ");
  Serial.print(timeoutMs);
  Serial.println(" ms");
  
  Serial.println("[WiFiManager] Setting WiFi mode to STA...");
  WiFi.mode(WIFI_STA);
  delay(100);
  
  Serial.println("[WiFiManager] Calling WiFi.begin()...");
  WiFi.begin(ssid.c_str(), password.c_str());
  
  Serial.print("[WiFiManager] Connecting to WiFi");
  unsigned long startTime = millis();
  int attempts = 0;
  int lastStatus = -1;
  
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeoutMs) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    int currentStatus = WiFi.status();
    if (currentStatus != lastStatus) {
      Serial.println();
      Serial.print("[WiFiManager] WiFi status changed to: ");
      Serial.println(currentStatus);
      lastStatus = currentStatus;
      Serial.print("[WiFiManager] Connecting");
    }
    
    if (server) {
      server->handleClient();
    }
  }
  
  Serial.println();
  unsigned long elapsedTime = millis() - startTime;
  
  Serial.print("[WiFiManager] Connection attempts: ");
  Serial.println(attempts);
  Serial.print("[WiFiManager] Time elapsed: ");
  Serial.print(elapsedTime);
  Serial.println(" ms");
  Serial.print("[WiFiManager] Final WiFi status: ");
  Serial.println(WiFi.status());
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFiManager] SUCCESS - WiFi connected!");
    Serial.print("[WiFiManager] IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("[WiFiManager] FAILED - WiFi connection failed");
    Serial.println("[WiFiManager] Status codes:");
    Serial.println("[WiFiManager]   0 = WL_IDLE_STATUS");
    Serial.println("[WiFiManager]   1 = WL_NO_SSID_AVAIL");
    Serial.println("[WiFiManager]   2 = WL_SCAN_COMPLETED");
    Serial.println("[WiFiManager]   3 = WL_CONNECTED");
    Serial.println("[WiFiManager]   4 = WL_CONNECT_FAILED");
    Serial.println("[WiFiManager]   5 = WL_CONNECTION_LOST");
    Serial.println("[WiFiManager]   6 = WL_DISCONNECTED");
    return false;
  }
}

void WiFiManager::loadCredentials() {
  Serial.println("[WiFiManager] Loading credentials from EEPROM...");
  ssid = readStringFromEEPROM(SSID_ADDR, WIFI_SSID_MAXLEN);
  password = readStringFromEEPROM(PASS_ADDR, WIFI_PASS_MAXLEN);
  
  credentialsFound = (ssid.length() > 0 && password.length() > 0);
  
  Serial.print("[WiFiManager] SSID loaded: '");
  Serial.print(ssid);
  Serial.println("'");
  Serial.print("[WiFiManager] SSID length: ");
  Serial.println(ssid.length());
  Serial.print("[WiFiManager] Password length: ");
  Serial.println(password.length());
  Serial.print("[WiFiManager] Credentials found: ");
  Serial.println(credentialsFound ? "YES" : "NO");
}

void WiFiManager::writeStringToEEPROM(int address, String data, int maxLen) {
  Serial.print("[WiFiManager] Writing to EEPROM at address ");
  Serial.print(address);
  Serial.print(", length ");
  Serial.println(data.length());
  
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
