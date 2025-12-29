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

// Configuration
const uint16_t CROSS_ID = 1;  // Change this for each device
const char* AP_SSID = "cross.1";
const char* AP_PASSWORD = "";  // Open AP (no password)
const char* HOSTNAME = "cross-1";

// Network setup
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

// n8n API endpoint (change to your actual endpoint)
const char* API_ENDPOINT = "https://n8n.app.p7n.net/webhook/c570d860-81e0-4e95-b04d-13014f3b373c/crosses";
const char* API_CROSS_ID = "2";  // Change to your cross ID

// Forward declarations
bool connectToStoredWiFi();
void startAPMode();
void startmDNS();
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
  Serial.println("Cross ID: " + String(CROSS_ID));
  
  // Initialize preferences (EEPROM wrapper)
  preferences.begin("cross", false);
  
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
    // Connected mode - handle web requests and poll API
    webServer.handleClient();
    
    // Poll API every 60 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastPollTime >= POLL_INTERVAL) {
      lastPollTime = currentTime;
      pollAPI();
    }
    
    delay(100);
  }
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
  WiFi.softAP(AP_SSID, AP_PASSWORD);
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
  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("Error starting mDNS");
    return;
  }
  
  Serial.println("mDNS started: " + String(HOSTNAME) + ".local");
  MDNS.addService("http", "tcp", 80);
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
  
  String url = String(API_ENDPOINT) + "?crossid=" + String(API_CROSS_ID);
  
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
