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
  
  if (!server) {
    Serial.println("[WiFiManager] ERROR: Server allocation failed! Aborting.");
    return false;
  }
  
  Serial.println("[WiFiManager] Access point is ready for connections.");
  Serial.println("[WiFiManager] Connect to 'AirDMX-Setup' and browse to 192.168.10.1");
  Serial.println("[WiFiManager] Waiting for user to configure WiFi...");
  Serial.print("[WiFiManager] Waiting for up to ");
  Serial.print(timeoutMs / 1000);
  Serial.println(" seconds...");
  
  // Wait for user to configure WiFi
  unsigned long startTime = millis();
  unsigned long lastLogTime = startTime;
  int logIntervalMs = 5000; // Log every 5 seconds
  
  while (!isConnected() && (millis() - startTime) < timeoutMs) {
    // Handle client requests
    if (server) {
      server->handleClient();
#ifdef ESP8266
      yield();
#endif
    }
    
    // Log progress every 5 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastLogTime >= logIntervalMs) {
      Serial.print("[WiFiManager] Still waiting... ");
      Serial.print((currentTime - startTime) / 1000);
      Serial.println(" seconds elapsed");
      
      Serial.print("[WiFiManager] AP clients connected: ");
      Serial.println(WiFi.softAPgetStationNum());
      
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
  IPAddress apIP(192, 168, 10, 1);
  bool ipConfigured = WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  Serial.print("[WiFiManager] AP IP configured: ");
  Serial.println(ipConfigured ? "SUCCESS" : "FAILED");
  
  Serial.println("\n========== ACCESS POINT INFO ==========");
  Serial.println("[WiFiManager] Access Point started");
  Serial.println("[WiFiManager] SSID: AirDMX-Setup");
  Serial.println("[WiFiManager] Password: airdmx123");
  Serial.println("[WiFiManager] IP Address: 192.168.10.1");
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
  
  Serial.print("[WiFiManager] Free heap before route setup: ");
  Serial.println(ESP.getFreeHeap());
  
  Serial.println("[WiFiManager] Setting up route handlers...");
  server->on("/", [this]() { handleRoot(); });
  server->on("/scan", [this]() { handleScan(); });
  server->on("/save", [this]() { handleSave(); });
  server->onNotFound([this]() { handleNotFound(); });
  
  Serial.println("[WiFiManager] Starting web server...");
  server->begin();
  
  Serial.print("[WiFiManager] Free heap after server start: ");
  Serial.println(ESP.getFreeHeap());
  
  Serial.println("[WiFiManager] Web server started on port 80");
}

void WiFiManager::handleRoot() {
  Serial.println("[WiFiManager] Handling / request");
  
  String html = "<!DOCTYPE html><html><head><title>AirDMX Setup</title><style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0}";
  html += ".container{background:white;padding:20px;border-radius:5px;max-width:400px;margin:0 auto}";
  html += "h1{color:#333}input,button{width:100%;padding:10px;margin:10px 0;box-sizing:border-box}";
  html += "button{background:#4CAF50;color:white;border:none;border-radius:4px;cursor:pointer}";
  html += ".network{padding:8px;margin:5px 0;background:#f9f9f9;border:1px solid #ddd;border-radius:3px;cursor:pointer}";
  html += ".network:hover{background:#f0f0f0}</style></head><body>";
  html += "<div class=\"container\"><h1>AirDMX WiFi</h1><p>Connect to a WiFi network:</p>";
  html += "<div id=\"networks\" style=\"border:1px solid #ddd;padding:10px;max-height:200px;overflow-y:auto\">";
  html += "Scanning...</div><form method=\"POST\" action=\"/save\">";
  html += "<input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"SSID\" required>";
  html += "<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Password\" required>";
  html += "<button type=\"submit\">Connect</button></form></div>";
  html += "<script>fetch('/scan').then(r=>r.json()).then(d=>{let h='';d.forEach(n=>{";
  html += "h+='<div class=\"network\" onclick=\"document.getElementById(\\'ssid\\').value=\\''+n.ssid+'\\';document.getElementById(\\'password\\').focus()\">'";
  html += "+n.ssid+'</div>'});document.getElementById('networks').innerHTML=h});";
  html += "</script></body></html>";
  
  server->send(200, "text/html", html);
  Serial.println("[WiFiManager] Root response sent");
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
    
    // Escape quotes in SSID
    ssidName.replace("\"", "");
    
    json += "{\"ssid\":\"" + ssidName + "\",\"rssi\":" + String(rssi) + "}";
  }
  json += "]";
  
  server->send(200, "application/json", json);
  Serial.print("[WiFiManager] Scan response sent: ");
  Serial.print(json.length());
  Serial.println(" bytes");
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
  
  String response = "<html><body><h1>Connecting...</h1>";
  response += "<p>Attempting to connect to: <b>" + newSSID + "</b></p>";
  response += "<p>Please wait...</p></body></html>";
  
  server->send(200, "text/html", response);
  Serial.println("[WiFiManager] Save response sent");
  
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
      Serial.print("[WiFiManager] WiFi status: ");
      Serial.println(currentStatus);
      lastStatus = currentStatus;
      Serial.print("[WiFiManager] Connecting");
    }
    
    if (server) {
      server->handleClient();
    }
#ifdef ESP8266
    yield();
#endif
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
