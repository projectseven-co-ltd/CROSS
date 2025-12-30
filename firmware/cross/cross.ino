/*
 * CROSS Prayer Network - ESP32 AP Mode + mDNS Discovery
 * WiFi setup via Access Point with REST API, mDNS for network discovery
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
const uint16_t DEFAULT_CROSS_ID = 1;
const uint16_t DEFAULT_GEO_ID = 1;
const char* FIRMWARE_VERSION = "3.3.0";
const char* GITHUB_API_URL = "https://api.github.com/repos/YOUR_USERNAME/CROSS/releases/latest";

const char* BIBLICAL_NAMES[] = {
  "luke", "john", "mark", "matthew", "paul", "peter", "james", "timothy",
  "david", "daniel", "samuel", "joseph", "moses", "joshua", "elijah", "isaac",
  "jacob", "noah", "abraham", "solomon", "adam", "ezra", "jonah", "micah"
};
const int BIBLICAL_NAMES_COUNT = 24;

uint16_t CROSS_ID;
uint16_t GEO_ID;
String DEVICE_NAME;
String HOSTNAME;      // Display name (e.g., luke3a7.cross)
String MDNS_HOSTNAME; // Actual mDNS name (e.g., luke3a7.local)
String AP_SSID;
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;

const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

bool isConnected = false;
bool apModeActive = false;

// Forward declarations for captive portal handlers
void handlePortal();
void handlePortalSave();
void handleOTAUpdate();
void checkForUpdate();

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("CROSS v" + String(FIRMWARE_VERSION));
  preferences.begin("cross", false);
  loadDeviceConfig();
  
  // Try to connect to saved WiFi
  if (!connectToSavedWiFi()) {
    startAPMode();
  } else {
    startmDNS();
  }
  
  Serial.println("READY");
}

void loop() {
  if (apModeActive) {
    dnsServer.processNextRequest();
    server.handleClient();
  }
  delay(1);
}

// ============================================================================
// CONFIG
// ============================================================================

void loadDeviceConfig() {
  CROSS_ID = preferences.getUShort("cross_id", DEFAULT_CROSS_ID);
  if (CROSS_ID == 0) {
    CROSS_ID = DEFAULT_CROSS_ID;
    preferences.putUShort("cross_id", CROSS_ID);
  }
  
  GEO_ID = preferences.getUShort("geo_id", DEFAULT_GEO_ID);
  if (GEO_ID == 0) {
    GEO_ID = DEFAULT_GEO_ID;
    preferences.putUShort("geo_id", GEO_ID);
  }
  
  DEVICE_NAME = preferences.getString("device_name", "");
  if (DEVICE_NAME == "") {
    randomSeed(ESP.getEfuseMac());
    String name = String(BIBLICAL_NAMES[random(BIBLICAL_NAMES_COUNT)]);
    String chars = "";
    const char* charset = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < 3; i++) {
      chars += charset[random(36)];
    }
    DEVICE_NAME = name + chars;
    preferences.putString("device_name", DEVICE_NAME);
  }
  
  AP_SSID = DEVICE_NAME; // Use same name for SSID and mDNS to avoid captive portal confusion
  HOSTNAME = DEVICE_NAME + ".cross";      // Display name for UI
  MDNS_HOSTNAME = DEVICE_NAME + ".local"; // mDNS requires .local TLD
  
  Serial.println("CFG id=" + String(CROSS_ID) + " name=" + DEVICE_NAME + " mdns=" + MDNS_HOSTNAME + " ap_ssid=" + AP_SSID);
}

// ============================================================================
// WIFI
// ============================================================================

bool connectToSavedWiFi() {
  String ssid = preferences.getString("wifi_ssid", "");
  if (ssid == "") {
    Serial.println("[WiFi] No saved credentials");
    return false;
  }
  
  String pwd = preferences.getString("wifi_password", "");
  Serial.println("[WiFi] Connecting to: " + ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pwd.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    isConnected = true;
    Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());
    return true;
  }
  
  Serial.println("\n[WiFi] Failed, starting AP mode");
  return false;
}

void startAPMode() {
  Serial.println("AP start ssid=" + AP_SSID);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID.c_str());
  dnsServer.start(DNS_PORT, "*", AP_IP); // Redirect all DNS to the portal
  
  apModeActive = true;
  
  // Start mDNS - NOTE: mDNS only works with .local TLD (RFC 6762)
  MDNS.begin(DEVICE_NAME.c_str());
  MDNS.addService("http", "tcp", 80);

  // Captive portal endpoints
  server.on("/", HTTP_GET, handlePortal);
  server.on("/save", HTTP_POST, handlePortalSave);
  server.onNotFound(handlePortal);

  // Legacy API (kept for app compatibility)
  server.on("/api/status", HTTP_GET, handleStatusAPI);
  server.on("/api/scan-wifi", HTTP_GET, handleScanWiFiAPI);
  server.on("/api/configure-wifi", HTTP_POST, handleConfigureWiFiAPI);
  server.on("/api/update", HTTP_POST, handleOTAUpdate);
  
  server.begin();
  
  Serial.println("AP ip=192.168.4.1 ssid=" + AP_SSID);
  Serial.println("Portal http://192.168.4.1/ or http://" + MDNS_HOSTNAME + "/");
}

// ============================================================================
// API ENDPOINTS
// ============================================================================

void handleStatusAPI() {
  DynamicJsonDocument doc(256);
  doc["id"] = CROSS_ID;
  doc["name"] = DEVICE_NAME;
  doc["version"] = FIRMWARE_VERSION;
  doc["mode"] = "ap";
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", response);
}

void handleScanWiFiAPI() {
  Serial.println("scan wifi");
  
  int networks = WiFi.scanNetworks();
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  
  for (int i = 0; i < networks; i++) {
    JsonObject net = arr.createNestedObject();
    net["ssid"] = WiFi.SSID(i);
    
    int sec = WiFi.encryptionType(i);
    String encryption = "open";
    if (sec == WIFI_AUTH_WEP) encryption = "WEP";
    else if (sec == WIFI_AUTH_WPA_PSK) encryption = "WPA";
    else if (sec == WIFI_AUTH_WPA2_PSK) encryption = "WPA2";
    else if (sec == WIFI_AUTH_WPA_WPA2_PSK) encryption = "WPA/WPA2";
    else if (sec == WIFI_AUTH_WPA2_ENTERPRISE) encryption = "WPA2-Enterprise";
    else if (sec == WIFI_AUTH_WPA3_PSK) encryption = "WPA3";
    
    net["encryption"] = encryption;
    net["rssi"] = WiFi.RSSI(i);
  }
  
  String response;
  serializeJson(arr, response);
  
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", response);
}

// ============================================================================
// CAPTIVE PORTAL
// ============================================================================

void handlePortal() {
  String page;
  page.reserve(2048);
  page += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>CROSS Setup</title>";
  page += "<style>body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:16px;}";
  page += ".card{background:#181818;border:1px solid #333;border-radius:8px;padding:16px;max-width:420px;width:100%;margin:auto;box-sizing:border-box;}";
  page += "label{display:block;margin:12px 0 6px;}input,select{width:100%;max-width:100%;padding:10px;border:1px solid #444;border-radius:6px;background:#000;color:#fff;box-sizing:border-box;}";
  page += "button{width:100%;max-width:100%;padding:12px;margin-top:14px;background:#fff;color:#000;border:none;border-radius:6px;font-weight:bold;box-sizing:border-box;}";
  page += "h1,h2,p{margin:0 0 12px 0;}</style></head><body><div class='card'>";
  page += "<h1>CROSS</h1><p>Configure WiFi</p>";
  
  // WiFi scan
  int networks = WiFi.scanNetworks();
  page += "<form method='POST' action='/save'>";
  page += "<label>WiFi Network</label><select name='ssid' required>";
  if (networks <= 0) {
    page += "<option value=''>No networks found</option>";
  } else {
    for (int i = 0; i < networks; i++) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      page += "<option value='" + ssid + "'>" + ssid + " (" + String(rssi) + " dBm)</option>";
    }
  }
  page += "</select>";
  page += "<label>Password</label><input name='password' type='password' placeholder='optional if open'>";
  page += "<button type='submit'>Save & Restart</button>";
  page += "</form><p style='margin-top:12px;font-size:12px;color:#aaa;'>Device: " + DEVICE_NAME + "</p>";
  page += "</div></body></html>";
  
  server.sendHeader("Content-Type", "text/html");
  server.send(200, "text/html", page);
}

void handlePortalSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID required");
    return;
  }
  
  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_password", password);
  
  String msg = "<html><body><h2>Saved.</h2><p>Rebooting...</p></body></html>";
  server.sendHeader("Content-Type", "text/html");
  server.send(200, "text/html", msg);
  delay(800);
  ESP.restart();
}

void handleConfigureWiFiAPI() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\": \"No body\"}");
    return;
  }
  
  DynamicJsonDocument doc(256);
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  
  if (err) {
    server.send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
    return;
  }
  
  String ssid = doc["ssid"] | "";
  String password = doc["password"] | "";
  
  if (ssid == "") {
    server.send(400, "application/json", "{\"error\": \"SSID required\"}");
    return;
  }
  
  Serial.println("store wifi ssid=" + ssid);
  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_password", password);
  
  DynamicJsonDocument response(128);
  response["status"] = "configured";
  response["message"] = "Device will restart";
  
  String responseStr;
  serializeJson(response, responseStr);
  
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", responseStr);
  
  // Restart
  delay(1000);
  ESP.restart();
}

// ============================================================================
// mDNS
// ============================================================================

void startmDNS() {
  if (MDNS.begin(DEVICE_NAME.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS http://" + MDNS_HOSTNAME + "/api/");
  }
}

// ============================================================================
// OTA UPDATE
// ============================================================================

void handleOTAUpdate() {
  Serial.println("OTA update requested");
  
  if (!isConnected) {
    server.send(400, "application/json", "{\"error\": \"Device must be on WiFi for OTA\"}");
    return;
  }
  
  server.send(200, "application/json", "{\"status\": \"updating\", \"message\": \"Downloading firmware...\"}");
  delay(100);
  
  checkForUpdate();
}

void checkForUpdate() {
  HTTPClient http;
  
  Serial.println("Checking for firmware update...");
  Serial.println("Current version: " + String(FIRMWARE_VERSION));
  
  // Check version first
  http.begin(GITHUB_VERSION_URL);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String latestVersion = http.getString();
    latestVersion.trim();
    Serial.println("Latest version: " + latestVersion);
    
    if (latestVersion != String(FIRMWARE_VERSION)) {
      Serial.println("New version available, updating...");
      http.end();
      
      // Perform update
      WiFiClient client;
      t_httpUpdate_return ret = httpUpdate.update(client, GITHUB_FIRMWARE_URL);
      
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.println("Update failed: " + String(httpUpdate.getLastErrorString()));
          break;
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("No update needed");
          break;
        case HTTP_UPDATE_OK:
          Serial.println("Update successful, rebooting...");
          ESP.restart();
          break;
      }
    } else {
      Serial.println("Already on latest version");
    }
  } else {
    Serial.println("Failed to check version: " + String(httpCode));
  }
  
  http.end();
}
