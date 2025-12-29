/*
 * CROSS Prayer Network - ESP32 Nano WiFi Onboarding
 * Captive Portal + mDNS + Credential Storage + API Polling
 * 
 * This sketch sets up a captive portal for WiFi configuration,
 * advertises via mDNS, persists credentials in EEPROM, and polls
 * the n8n API for heartbeat/status updates every 60 seconds.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>

// ============================================================================
// DEVICE CONFIGURATION DEFAULTS
// These are only used on FIRST BOOT if not already stored in EEPROM
// ============================================================================
const uint16_t DEFAULT_CROSS_ID = 1;      // Cross ID (must match database)
const uint16_t DEFAULT_GEO_ID = 1;        // Geographic Area ID (must match geo_areas table)

// Device configuration (loaded from Preferences/EEPROM at boot)
uint16_t CROSS_ID;
uint16_t GEO_ID;

// Derived configuration (auto-generated from CROSS_ID)
String AP_SSID;
String HOSTNAME;

// ============================================================================
// FIRMWARE & API CONFIGURATION
// ============================================================================
const char* FIRMWARE_VERSION = "1.0.0";  // Current firmware version
const char* GITHUB_REPO_OWNER = "projectseven-co-ltd";
const char* GITHUB_REPO_NAME = "CROSS-firmware";
const char* GITHUB_RELEASE_API = "https://api.github.com/repos/projectseven-co-ltd/CROSS-firmware/releases/latest";
const char* FIRMWARE_BIN_NAME = "cross.bin";

// n8n API endpoint
const char* API_ENDPOINT = "https://n8n.app.p7n.net/webhook/c570d860-81e0-4e95-b04d-13014f3b373c/crosses";

// ============================================================================
// NETWORK SETUP
// ============================================================================
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// DNS and Web Server
DNSServer dnsServer;
WebServer webServer(80);
Preferences preferences;  // For storing WiFi credentials

// WiFi state
bool isConnected = false;
bool setupMode = false;

// Polling and heartbeat
unsigned long lastPollTime = 0;
const unsigned long POLL_INTERVAL = 60000;  // 60 seconds (1 minute)
unsigned long lastHeartbeatTime = 0;
int lastSignalStrength = 0;
String lastErrorMessage = "";

// OTA update checking
unsigned long lastOTACheck = 0;
const unsigned long OTA_CHECK_INTERVAL = 3600000;  // 1 hour in milliseconds
bool isUpdating = false;

// Forward declarations
void loadDeviceConfig();
bool connectToStoredWiFi();
void startAPMode();
void startmDNS();
void checkForFirmwareUpdate();
void performOTAUpdate(String firmwareURL);
void handleRoot();
void handleStatus();
void handleSetup();
void handleNotFound();
void pollAPI();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\nCROSS Prayer Network - ESP32");
  Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
  
  // Initialize preferences (EEPROM wrapper)
  preferences.begin("cross", false);
  
  // Load device configuration from Preferences (persists across firmware updates)
  loadDeviceConfig();
  
  Serial.println("Cross ID: " + String(CROSS_ID));
  Serial.println("Geo Area ID: " + String(GEO_ID));
  
  // Attempt to connect to stored WiFi
  if (!connectToStoredWiFi()) {
    Serial.println("No valid stored WiFi or connection failed. Starting AP mode.");
    startAPMode();
  }
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  if (setupMode) {
    // Captive portal mode
    dnsServer.processNextRequest();
    webServer.handleClient();
  } else if (isConnected) {
    // Connected mode - handle web requests, check for updates, and poll API
    if (!isUpdating) {
      webServer.handleClient();
      
      unsigned long currentTime = millis();
      
      // Poll API every 60 seconds
      if (currentTime - lastPollTime >= POLL_INTERVAL) {
        lastPollTime = currentTime;
        pollAPI();
      }
      
      // Check for firmware updates every hour
      if (currentTime - lastOTACheck >= OTA_CHECK_INTERVAL) {
        lastOTACheck = currentTime;
        checkForFirmwareUpdate();
      }
    }
    
    delay(100);
  }
}

// ============================================================================
// Device Configuration Functions
// ============================================================================

void loadDeviceConfig() {
  // Load device configuration from Preferences (EEPROM)
  // If not set, use defaults and save them
  
  CROSS_ID = preferences.getUShort("cross_id", 0);
  if (CROSS_ID == 0) {
    // First boot - use defaults and save
    CROSS_ID = DEFAULT_CROSS_ID;
    preferences.putUShort("cross_id", CROSS_ID);
    Serial.println("[CONFIG] First boot - saved CROSS_ID: " + String(CROSS_ID));
  }
  
  GEO_ID = preferences.getUShort("geo_id", 0);
  if (GEO_ID == 0) {
    // First boot - use defaults and save
    GEO_ID = DEFAULT_GEO_ID;
    preferences.putUShort("geo_id", GEO_ID);
    Serial.println("[CONFIG] First boot - saved GEO_ID: " + String(GEO_ID));
  }
  
  // Generate derived configuration
  AP_SSID = "cross." + String(CROSS_ID);
  HOSTNAME = "cross-" + String(CROSS_ID);
  
  Serial.println("[CONFIG] Loaded from EEPROM - survives firmware updates");
}

// ============================================================================
// WiFi Connection Functions
// ============================================================================

bool connectToStoredWiFi() {
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  
  if (ssid.length() == 0) {
    Serial.println("No stored WiFi credentials.");
    return false;
  }
  
  Serial.println("Attempting to connect to stored WiFi: " + ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // Wait up to 10 seconds for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    isConnected = true;
    setupMode = false;
    
    // Start mDNS
    startmDNS();
    
    // Register web server routes for connected mode
    webServer.on("/", handleStatus);
    webServer.on("/status", handleStatus);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
    
    Serial.println("Web server started on port 80");
    
    // Check for firmware updates on boot
    checkForFirmwareUpdate();
    lastOTACheck = millis();  // Reset the timer
    
    return true;
  }
  
  Serial.println("\nFailed to connect to stored WiFi.");
  return false;
}

void startAPMode() {
  isConnected = false;
  setupMode = true;
  
  // Stop STA mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID.c_str(), "");
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  
  Serial.println("AP Mode started:");
  Serial.println("SSID: " + String(AP_SSID));
  Serial.println("IP: " + AP_IP.toString());
  
  // Start DNS server (captive portal redirect)
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", AP_IP);
  
  // Setup web server routes for setup mode
  webServer.on("/", handleRoot);
  webServer.on("/api/setup", HTTP_POST, handleSetup);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  
  Serial.println("Web server started on port 80");
}

void startmDNS() {
  if (!MDNS.begin(HOSTNAME.c_str())) {
    Serial.println("Error starting mDNS");
    return;
  }
  
  Serial.println("mDNS started: " + String(HOSTNAME) + ".local");
  MDNS.addService("http", "tcp", 80);
}

// ============================================================================
// GitHub-Based OTA Updates
// ============================================================================

void checkForFirmwareUpdate() {
  if (!isConnected) return;
  
  Serial.println("[OTA] Checking for firmware updates...");
  Serial.println("[OTA] Current version: " + String(FIRMWARE_VERSION));
  
  // Quick DNS + TCP sanity check before full HTTPS request
  IPAddress resolvedIP;
  if (!WiFi.hostByName("api.github.com", resolvedIP)) {
    Serial.println("[OTA] DNS resolution failed for api.github.com");
    return;
  }
  Serial.println("[OTA] api.github.com -> " + resolvedIP.toString());
  
  WiFiClientSecure testClient;
  testClient.setInsecure();
  testClient.setTimeout(15000);
  if (!testClient.connect("api.github.com", 443)) {
    char errBuf[80];
    int err = testClient.lastError(errBuf, sizeof(errBuf));
    Serial.println("[OTA] TCP/TLS connect failed to api.github.com:443");
    Serial.printf("[OTA] TLS lastError=%d (%s)\n", err, errBuf);
    return;
  }
  testClient.stop();
  
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();  // For GitHub API; consider adding certificate validation in production
  client.setTimeout(15000);
  
  // Build GitHub API URL
  char apiURL[256];
  snprintf(apiURL, sizeof(apiURL), GITHUB_RELEASE_API, GITHUB_REPO_OWNER, GITHUB_REPO_NAME);
  
  http.begin(client, apiURL);
  http.setTimeout(15000);
  http.useHTTP10();  // Avoid chunked encoding issues
  http.addHeader("User-Agent", "CROSS-ESP32");
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    // Simple JSON parsing to extract tag_name (version) and download URL
    int tagIndex = payload.indexOf("\"tag_name\":\"");
    if (tagIndex > 0) {
      tagIndex += 12;
      int tagEnd = payload.indexOf("\"", tagIndex);
      String latestVersion = payload.substring(tagIndex, tagEnd);
      
      // Remove 'v' prefix if present (e.g., v1.0.1 -> 1.0.1)
      if (latestVersion.startsWith("v")) {
        latestVersion = latestVersion.substring(1);
      }
      
      Serial.println("[OTA] Latest version: " + latestVersion);
      
      // Compare versions (simple string comparison works for semantic versioning)
      if (latestVersion != String(FIRMWARE_VERSION)) {
        Serial.println("[OTA] New version available! Preparing to update...");
        
        // Find the firmware binary download URL
        String searchPattern = "\"browser_download_url\":\"";
        int urlIndex = payload.indexOf(searchPattern);
        
        while (urlIndex > 0) {
          urlIndex += searchPattern.length();
          int urlEnd = payload.indexOf("\"", urlIndex);
          String downloadURL = payload.substring(urlIndex, urlEnd);
          
          // Check if this is the firmware binary we want
          if (downloadURL.endsWith(FIRMWARE_BIN_NAME)) {
            Serial.println("[OTA] Download URL: " + downloadURL);
            http.end();
            performOTAUpdate(downloadURL);
            return;
          }
          
          // Look for next URL
          urlIndex = payload.indexOf(searchPattern, urlEnd);
        }
        
        Serial.println("[OTA] Firmware binary not found in release assets");
      } else {
        Serial.println("[OTA] Firmware is up to date");
      }
    } else {
      Serial.println("[OTA] Could not parse release information");
    }
  } else {
    Serial.println("[OTA] GitHub API request failed. HTTP code: " + String(httpCode));
  }
  
  http.end();
}

void performOTAUpdate(String firmwareURL) {
  Serial.println("[OTA] Starting firmware download...");
  isUpdating = true;
  
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);
  
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // handle 302 redirects
  http.useHTTP10();                                        // avoid chunked transfer issues
  http.setTimeout(15000);
  http.addHeader("User-Agent", "CROSS-ESP32");
  
  http.begin(client, firmwareURL);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    int contentLength = http.getSize();
    Serial.println("[OTA] Firmware size: " + String(contentLength) + " bytes");
    
    if (contentLength > 0) {
      bool canBegin = Update.begin(contentLength);
      
      if (canBegin) {
        Serial.println("[OTA] Begin update. This will take a minute...");
        
        WiFiClient * stream = http.getStreamPtr();
        size_t written = Update.writeStream(*stream);
        
        if (written == contentLength) {
          Serial.println("[OTA] Written : " + String(written) + " successfully");
        } else {
          Serial.println("[OTA] Written only : " + String(written) + "/" + String(contentLength));
        }
        
        if (Update.end()) {
          Serial.println("[OTA] Update complete!");
          if (Update.isFinished()) {
            Serial.println("[OTA] Update successfully completed. Rebooting...");
            delay(1000);
            ESP.restart();
          } else {
            Serial.println("[OTA] Update not finished. Something went wrong!");
          }
        } else {
          Serial.println("[OTA] Error Occurred. Error #: " + String(Update.getError()));
        }
      } else {
        Serial.println("[OTA] Not enough space to begin OTA");
      }
    } else {
      Serial.println("[OTA] Invalid content length");
    }
  } else {
    Serial.println("[OTA] Firmware download failed. HTTP code: " + String(httpCode));
  }
  
  http.end();
  isUpdating = false;
}

// ============================================================================
// Web Server Handlers
// ============================================================================

void handleRoot() {
  // WiFi setup form with network scanning and dark theme
  Serial.println("GET /");
  
  // Scan for WiFi networks
  Serial.println("Scanning WiFi networks...");
  int networkCount = WiFi.scanNetworks();
  Serial.println("Found " + String(networkCount) + " networks");
  
  String networkOptions = "";
  if (networkCount > 0) {
    for (int i = 0; i < networkCount; i++) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      String signal = (rssi > -50) ? "Strong" : (rssi > -70) ? "Good" : "Weak";
      networkOptions += "<option value='" + ssid + "'>" + ssid + " (" + signal + ")</option>";
    }
  }
  
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>CROSS WiFi Setup</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body {";
  html += "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;";
  html += "  background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);";
  html += "  display: flex;";
  html += "  justify-content: center;";
  html += "  align-items: center;";
  html += "  min-height: 100vh;";
  html += "  padding: 20px;";
  html += "}";
  html += ".container {";
  html += "  background: #2d2d44;";
  html += "  padding: 40px;";
  html += "  border-radius: 15px;";
  html += "  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.5);";
  html += "  max-width: 450px;";
  html += "  width: 100%;";
  html += "  border: 1px solid #3d3d5c;";
  html += "}";
  html += "h1 { text-align: center; color: #ffffff; margin-bottom: 8px; font-size: 28px; }";
  html += ".subtitle { text-align: center; color: #b0b0c0; margin-bottom: 30px; font-size: 14px; }";
  html += ".form-group { margin-bottom: 20px; }";
  html += "label { display: block; margin-bottom: 8px; color: #e0e0e0; font-weight: 600; font-size: 14px; }";
  html += "input, select { width: 100%; padding: 12px 14px; border: 1px solid #4d4d6d; border-radius: 8px; background: #1a1a2e; color: #ffffff; font-size: 15px; transition: all 0.3s ease; }";
  html += "input:focus, select:focus { outline: none; border-color: #667eea; background: #252535; box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.2); }";
  html += "select option { background: #1a1a2e; color: #ffffff; }";
  html += "button { width: 100%; padding: 14px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; border-radius: 8px; font-size: 16px; font-weight: 600; cursor: pointer; transition: all 0.3s ease; text-transform: uppercase; letter-spacing: 1px; }";
  html += "button:hover { transform: translateY(-2px); box-shadow: 0 10px 25px rgba(102, 126, 234, 0.4); }";
  html += "button:active { transform: translateY(0); }";
  html += ".error { background: #3d1f1f; color: #ff6b6b; padding: 12px; border-radius: 6px; margin-bottom: 15px; display: none; font-size: 13px; }";
  html += ".error.show { display: block; }";
  html += ".success { background: #1f3d2f; color: #51cf66; padding: 12px; border-radius: 6px; margin-bottom: 15px; display: none; font-size: 13px; }";
  html += ".success.show { display: block; }";
  html += ".scan-info { font-size: 12px; color: #909090; margin-top: 5px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>CROSS</h1>";
  html += "<p class='subtitle'>Prayer Network WiFi Setup</p>";
  html += "<div class='error' id='errorMsg'></div>";
  html += "<div class='success' id='successMsg'></div>";
  html += "<form id='wifiForm'>";
  html += "<div class='form-group'>";
  html += "<label for='ssid'>Available Networks</label>";
  html += "<select id='ssid' name='ssid'>";
  html += "<option value=''>-- Select or Enter Network --</option>";
  html += networkOptions;
  html += "</select>";
  html += "<p class='scan-info'>Found " + String(networkCount) + " network(s)</p>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='ssidManual'>Or Enter Manually</label>";
  html += "<input type='text' id='ssidManual' placeholder='Enter network SSID'>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='password'>Password</label>";
  html += "<input type='password' id='password' name='password' placeholder='Leave blank if open network'>";
  html += "</div>";
  html += "<button type='submit'>Connect</button>";
  html += "</form>";
  html += "</div>";
  html += "<script>";
  html += "const ssidSelect = document.getElementById('ssid');";
  html += "const ssidManual = document.getElementById('ssidManual');";
  html += "ssidManual.addEventListener('input', function() { if (this.value) ssidSelect.value = ''; });";
  html += "ssidSelect.addEventListener('change', function() { if (this.value) ssidManual.value = ''; });";
  html += "document.getElementById('wifiForm').addEventListener('submit', async (e) => {";
  html += "  e.preventDefault();";
  html += "  const ssid = ssidSelect.value || ssidManual.value;";
  html += "  const password = document.getElementById('password').value;";
  html += "  const errorMsg = document.getElementById('errorMsg');";
  html += "  const successMsg = document.getElementById('successMsg');";
  html += "  errorMsg.classList.remove('show'); successMsg.classList.remove('show');";
  html += "  if (!ssid) { errorMsg.textContent = 'Please select or enter a network name'; errorMsg.classList.add('show'); return; }";
  html += "  try {";
  html += "    const response = await fetch('/api/setup', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ ssid, password }) });";
  html += "    const data = await response.json();";
  html += "    if (data.success) { successMsg.textContent = 'WiFi credentials saved! Device will restart...'; successMsg.classList.add('show'); setTimeout(() => location.reload(), 3000); }";
  html += "    else { errorMsg.textContent = 'Error: ' + data.message; errorMsg.classList.add('show'); }";
  html += "  } catch (err) { errorMsg.textContent = 'Connection error: ' + err; errorMsg.classList.add('show'); }";
  html += "});";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  
  webServer.send(200, "text/html", html);
}

void handleStatus() {
  // Show heartbeat/status page at root and /status
  Serial.println("GET /status");
  
  unsigned long timeSinceLastHeartbeat = (millis() - lastHeartbeatTime) / 1000;
  String uptime = String(millis() / 1000) + "s";
  String statusColor = (lastHeartbeatTime > 0 && timeSinceLastHeartbeat < 120) ? "#51cf66" : "#ff6b6b";
  String statusText = (lastHeartbeatTime > 0 && timeSinceLastHeartbeat < 120) ? "Connected" : "No Recent Heartbeat";
  
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>CROSS - Status</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%); color: #e0e0e0; padding: 20px; min-height: 100vh; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: #2d2d44; border: 1px solid #3d3d5c; border-radius: 12px; padding: 30px; box-shadow: 0 20px 60px rgba(0, 0, 0, 0.5); }";
  html += "h1 { text-align: center; color: #ffffff; margin-bottom: 10px; }";
  html += ".status-indicator { width: 20px; height: 20px; border-radius: 50%; background: " + statusColor + "; display: inline-block; margin-right: 10px; animation: pulse 2s infinite; }";
  html += "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }";
  html += ".status-line { margin: 15px 0; padding: 12px; background: #1a1a2e; border-left: 3px solid #667eea; border-radius: 4px; }";
  html += ".label { color: #909090; font-size: 12px; text-transform: uppercase; letter-spacing: 1px; }";
  html += ".value { color: #e0e0e0; font-size: 16px; font-weight: 600; margin-top: 5px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1><span class='status-indicator'></span> " + statusText + "</h1>";
  html += "<div class='status-line'>";
  html += "  <div class='label'>WiFi SSID</div>";
  html += "  <div class='value'>" + String(WiFi.SSID()) + "</div>";
  html += "</div>";
  html += "<div class='status-line'>";
  html += "  <div class='label'>IP Address</div>";
  html += "  <div class='value'>" + WiFi.localIP().toString() + "</div>";
  html += "</div>";
  html += "<div class='status-line'>";
  html += "  <div class='label'>Signal Strength</div>";
  html += "  <div class='value'>" + String(WiFi.RSSI()) + " dBm</div>";
  html += "</div>";
  html += "<div class='status-line'>";
  html += "  <div class='label'>Last Heartbeat</div>";
  html += "  <div class='value'>" + String(timeSinceLastHeartbeat) + " seconds ago</div>";
  html += "</div>";
  html += "<div class='status-line'>";
  html += "  <div class='label'>Device Uptime</div>";
  html += "  <div class='value'>" + uptime + "</div>";
  html += "</div>";
  html += "<div class='status-line'>";
  html += "  <div class='label'>Last Poll Status</div>";
  html += "  <div class='value'>" + (lastErrorMessage.length() > 0 ? "Error: " + lastErrorMessage : "Success") + "</div>";
  html += "</div>";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  
  webServer.send(200, "text/html", html);
}

void handleSetup() {
  // Handle WiFi credential submission
  Serial.println("POST /api/setup");
  
  if (!webServer.hasArg("plain")) {
    webServer.send(400, "application/json", "{\"success\": false, \"message\": \"No data provided\"}");
    return;
  }
  
  String body = webServer.arg("plain");
  
  // Simple JSON parsing (production would use ArduinoJson)
  int ssidStart = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd = body.indexOf("\"", ssidStart);
  String ssid = body.substring(ssidStart, ssidEnd);
  
  int passwordStart = body.indexOf("\"password\":\"") + 12;
  int passwordEnd = body.indexOf("\"", passwordStart);
  String password = body.substring(passwordStart, passwordEnd);
  
  Serial.println("Received WiFi: " + ssid);
  
  // Store in preferences
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  
  webServer.send(200, "application/json", "{\"success\": true, \"message\": \"Credentials saved. Restarting...\"}");
  
  delay(500);
  ESP.restart();
}

void handleNotFound() {
  // Redirect to root for captive portal
  if (setupMode) {
    webServer.sendHeader("Location", "http://192.168.4.1/", true);
    webServer.send(302, "text/plain", "");
  } else {
    webServer.send(404, "text/plain", "Not Found");
  }
}

// ============================================================================
// API Polling
// ============================================================================

void pollAPI() {
  if (!isConnected) return;
  
  Serial.println("[API] Polling heartbeat endpoint...");
  
  WiFiClientSecure client;
  client.setInsecure();  // For development; use proper certificates in production
  
  String url = String(API_ENDPOINT) + "?crossid=" + String(CROSS_ID);
  
  if (client.connect("n8n.app.p7n.net", 443)) {
    // Extract path from URL
    String path = url.substring(url.indexOf("/webhook"));
    
    String request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: n8n.app.p7n.net\r\n";
    request += "Connection: close\r\n";
    request += "User-Agent: CROSS-ESP32\r\n";
    request += "\r\n";
    
    client.print(request);
    
    unsigned long timeout = millis() + 5000;
    String response = "";
    bool foundOK = false;
    
    while (client.connected() && millis() < timeout) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        response += line;
        
        if (line.indexOf("200") > 0) {
          foundOK = true;
        }
      }
    }
    
    client.stop();
    
    if (foundOK) {
      Serial.println("[API] Heartbeat successful!");
      lastHeartbeatTime = millis();
      lastErrorMessage = "";
      lastSignalStrength = WiFi.RSSI();
    } else {
      Serial.println("[API] HTTP error in response");
      lastErrorMessage = "HTTP Error";
    }
  } else {
    Serial.println("[API] Connection to n8n.app.p7n.net failed");
    lastErrorMessage = "Connection failed";
  }
}
