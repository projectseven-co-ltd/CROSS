/*
 * CROSS WiFi Onboarding
 * Minimal firmware focused on WiFi provisioning via AP + captive portal/API.
 * Now uses periodic NocoDB polling for prayer events.
 */

// LED Strip Configuration - Choose ONE:
#define LED_TYPE_COMMON_ANODE    // Non-addressable RGB strip (3 pins: R, G, B)
// #define LED_TYPE_ADDRESSABLE  // WS2812/SK6812 addressable strip (1 data pin)

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

#ifdef LED_TYPE_ADDRESSABLE
#include <FastLED.h>
#endif

const char* FIRMWARE_VERSION = "3.3";
const char* GITHUB_API_URL = "https://api.github.com/repos/projectseven-co-ltd/CROSS-firmware/releases/latest";

// LED Strip Configuration
#ifdef LED_TYPE_COMMON_ANODE
  // Non-addressable RGB strip (common anode)
  #define LED_PIN_R 16       // GPIO pin for Red channel
  #define LED_PIN_G 17       // GPIO pin for Green channel
  #define LED_PIN_B 18       // GPIO pin for Blue channel
  #define PWM_CHANNEL_R 0    // PWM channel for Red
  #define PWM_CHANNEL_G 1    // PWM channel for Green
  #define PWM_CHANNEL_B 2    // PWM channel for Blue
  #define PWM_FREQ 5000      // PWM frequency
  #define PWM_RESOLUTION 8   // 8-bit resolution (0-255)
#endif

#ifdef LED_TYPE_ADDRESSABLE
  // Addressable LED strip (WS2812/SK6812)
  #define LED_DATA_PIN 16    // GPIO pin for data
  #define NUM_LEDS 1         // Number of LEDs in strip
  #define LED_TYPE_CHIP WS2812B
  #define COLOR_ORDER GRB
  CRGB leds[NUM_LEDS];
#endif

// NocoDB polling (set to your API and table/view endpoint)
const char* NOCODB_BASE_URL = "https://noco.app.p7n.net"; // e.g., https://nocodb.example.com
const char* NOCODB_PRAYERS_PATH = "/api/v2/tables/mt7iucs253e26u0/records"; // adjust if using a view
const char* NOCODB_CROSSES_PATH = "/api/v2/tables/mdk4garchpkpvsd/records";
const char* NOCODB_API_TOKEN = "fDhJb1s9aK8yQsj99iSt6DOe9o518yGAwAdwezn1"; // xc-token header
const uint32_t NOCODB_POLL_MS = 60000; // poll every 60s

const char* BIBLICAL_NAMES[] = {
  "luke", "john", "mark", "matthew", "paul", "peter", "james", "timothy",
  "david", "daniel", "samuel", "joseph", "moses", "joshua", "elijah", "isaac",
  "jacob", "noah", "abraham", "solomon", "adam", "ezra", "jonah", "micah"
};
const int BIBLICAL_NAMES_COUNT = 24;

String DEVICE_NAME;
String HOSTNAME;      // Display name (e.g., luke3a7.cross)
String MDNS_HOSTNAME; // Actual mDNS name (e.g., luke3a7.local)
String AP_SSID;
String DEVICE_KEY;    // Stable unique identifier derived from chip MAC
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;

const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

bool apModeActive = false;
bool serverStarted = false;
bool isConnected = false;

String crossId;

// NocoDB poll state
unsigned long lastNocoPollMs = 0;
String lastPrayerSeenTs;
long lastPrayerSeenId = 0;
long intercessionCount = 0;
long heartbeatCount = 0;  // Activity level counter (increments on each heartbeat)
bool versionSyncPending = false; // Tracks if firmware version needs DB sync
String versionSyncVersion;

// OTA update state
unsigned long lastOTACheckMs = 0;
const uint32_t OTA_CHECK_INTERVAL_MS = 86400000; // 24 hours in milliseconds

// Forward declarations
bool connectToSavedWiFi();
void startAPMode();
void loadDeviceConfig();
String computeDeviceKey();
void handlePortal();
void handlePortalSave();
void handleStatusAPI();
void handleScanWiFiAPI();
void handleConfigureWiFiAPI();
void handleWiFiReset();
void handleOTAUpdate();
void checkForUpdate();
void startHttpServer(bool inAPMode);
void startmDNS();
void runPrayerPulse();
void runAPModePulse();
void pollPrayersIfDue();
void pollPrayers();
void registerCrossIfNeeded();
bool fetchExistingCross();
bool createCrossRecord();
bool updateCrossVersionInNocoDB();
void attemptVersionSyncIfPending();
bool verifyCrossIdExists();

// ============================================================================
// SETUP & LOOP
// ============================================================================

// ============================================================================
// LED ABSTRACTION LAYER
// ============================================================================

void initLEDs() {
#ifdef LED_TYPE_COMMON_ANODE
  ledcAttach(LED_PIN_R, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(LED_PIN_G, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(LED_PIN_B, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(LED_PIN_R, 255);
  ledcWrite(LED_PIN_G, 255);
  ledcWrite(LED_PIN_B, 255);
#endif
#ifdef LED_TYPE_ADDRESSABLE
  FastLED.addLeds<LED_TYPE_CHIP, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
#endif
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
#ifdef LED_TYPE_COMMON_ANODE
  // Common anode: invert values (255 = off, 0 = on)
  ledcWrite(LED_PIN_R, 255 - r);
  ledcWrite(LED_PIN_G, 255 - g);
  ledcWrite(LED_PIN_B, 255 - b);
#endif
#ifdef LED_TYPE_ADDRESSABLE
  leds[0] = CRGB(r, g, b);
  FastLED.show();
#endif
}

void clearLEDs() {
#ifdef LED_TYPE_COMMON_ANODE
  ledcWrite(LED_PIN_R, 255);
  ledcWrite(LED_PIN_G, 255);
  ledcWrite(LED_PIN_B, 255);
#endif
#ifdef LED_TYPE_ADDRESSABLE
  FastLED.clear();
  FastLED.show();
#endif
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize LED strip
  initLEDs();
  // Subtle boot pulse to indicate power-on
  runBootPulse();

  Serial.println("CROSS WiFi Onboarding v" + String(FIRMWARE_VERSION));
  preferences.begin("cross", false);
  loadDeviceConfig();

  crossId = preferences.getString("cross_id", "");
  // restore last seen prayer timestamp
  lastPrayerSeenTs = preferences.getString("last_prayer_ts", "");
  lastPrayerSeenId = preferences.getLong("last_prayer_id", 0);
  intercessionCount = preferences.getLong("intercession_count", 0);
  heartbeatCount = preferences.getLong("heartbeat_count", 0);
  versionSyncPending = preferences.getBool("version_sync_pending", false);
  versionSyncVersion = preferences.getString("version_sync_version", "");

  if (!connectToSavedWiFi()) {
    startAPMode();
  } else {
    startmDNS();
    startHttpServer(false);
    syncTimeWithNTP();
    registerCrossIfNeeded();
    attemptVersionSyncIfPending();
    // Ensure NocoDB reflects current firmware on boot
    updateCrossVersionInNocoDB();
    // Check for OTA update on boot
    checkForUpdate();
  }

  Serial.println("READY");
}

void loop() {
  if (apModeActive) {
    dnsServer.processNextRequest();
    // Pulse yellow every 5 seconds to indicate AP mode
    static unsigned long lastAPPulse = 0;
    if (millis() - lastAPPulse > 5000) {
      lastAPPulse = millis();
      runAPModePulse();
    }
  }
  if (serverStarted) {
    server.handleClient();
  }
  if (!apModeActive && isConnected) {
    pollPrayersIfDue();
    checkForOTAUpdateIfDue();
  }
  delay(1);
}

// ============================================================================
// NTP TIME SYNC
// ============================================================================

void syncTimeWithNTP() {
  Serial.println("Syncing time with NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 24 * 3600 && attempts < 20) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    attempts++;
  }
  Serial.println();
  
  if (now > 24 * 3600) {
    Serial.println("Time synced: " + String(ctime(&now)));
  } else {
    Serial.println("NTP sync failed, time may be incorrect");
  }
}

// ============================================================================
// CONFIG
// ============================================================================

void loadDeviceConfig() {
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

  DEVICE_KEY = preferences.getString("device_key", "");
  if (DEVICE_KEY == "") {
    DEVICE_KEY = computeDeviceKey();
    preferences.putString("device_key", DEVICE_KEY);
  }

  AP_SSID = DEVICE_NAME;
  HOSTNAME = DEVICE_NAME + ".cross";
  MDNS_HOSTNAME = DEVICE_NAME + ".local";

  Serial.println("CFG name=" + DEVICE_NAME + " mdns=" + MDNS_HOSTNAME + " ap_ssid=" + AP_SSID);
}

String computeDeviceKey() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[17];
  snprintf(buf, sizeof(buf), "%012llX", (unsigned long long)mac);
  return String(buf);
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
  
  // Try up to 3 times with delays between attempts
  for (int attempt = 0; attempt < 3; attempt++) {
    WiFi.begin(ssid.c_str(), pwd.c_str());
    
    // Wait up to 20 seconds per attempt (40 x 500ms)
    int connect_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && connect_attempts < 40) {
      delay(500);
      Serial.print(".");
      connect_attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());
      apModeActive = false;
      isConnected = true;
      return true;
    }
    
    Serial.println("\n[WiFi] Attempt " + String(attempt + 1) + " failed, retrying...");
    delay(2000); // Wait 2 seconds before next attempt
  }

  Serial.println("[WiFi] Failed after 3 attempts, starting AP mode");
  return false;
}

void startAPMode() {
  Serial.println("AP start ssid=" + AP_SSID);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID.c_str());
  dnsServer.start(DNS_PORT, "*", AP_IP);

  apModeActive = true;

  MDNS.begin(DEVICE_NAME.c_str());
  MDNS.addService("http", "tcp", 80);

  startHttpServer(true);

  Serial.println("AP ip=192.168.4.1 ssid=" + AP_SSID);
  Serial.println("Portal http://192.168.4.1/ or http://" + MDNS_HOSTNAME + "/");
  
  // Indicate AP mode with yellow pulse
  runAPModePulse();
}

// ============================================================================
// LED INDICATORS
// ============================================================================

void runAPModePulse() {
  // AP mode indicator: slow, dark purple pulse
  const uint8_t pulseSpeed = 50;   // slower fade speed
  const uint8_t maxBrightness = 40; // darker purple cap
  
  // Dark purple RGB values (low R+B, no G)
  // Fade in
  for (uint8_t brightness = 0; brightness <= maxBrightness; brightness += 5) {
    uint8_t scaledValue = map(brightness, 0, maxBrightness, 0, 255);
    setLEDColor(scaledValue, 0, scaledValue);
    delay(pulseSpeed);
  }
  
  // Hold bright briefly
  delay(400);
  
  // Fade out
  for (uint8_t brightness = maxBrightness; brightness > 0; brightness -= 5) {
    uint8_t scaledValue = map(brightness, 0, maxBrightness, 0, 255);
    setLEDColor(scaledValue, 0, scaledValue);
    delay(pulseSpeed);
  }
  
  // Turn off
  clearLEDs();
}

void runPrayerPulse() {
  // Prayer pulse effect: fade in cyan, hold, fade out
  // Cyan color represents hope and intercession
  const uint8_t pulseSpeed = 20;  // milliseconds per brightness step
  const uint8_t maxBrightness = 100;
  
  // Cyan RGB values (0, 255, 255)
  // Fade in
  for (uint8_t brightness = 0; brightness <= maxBrightness; brightness += 5) {
    uint8_t scaledValue = map(brightness, 0, maxBrightness, 0, 255);
    setLEDColor(0, scaledValue, scaledValue);
    delay(pulseSpeed);
  }
  
  // Hold bright
  delay(500);
  
  // Fade out
  for (uint8_t brightness = maxBrightness; brightness > 0; brightness -= 5) {
    uint8_t scaledValue = map(brightness, 0, maxBrightness, 0, 255);
    setLEDColor(0, scaledValue, scaledValue);
    delay(pulseSpeed);
  }
  
  // Turn off
  clearLEDs();
  
  Serial.println("[LED] Prayer pulse complete");
}

void attemptVersionSyncIfPending() {
  if (!versionSyncPending && versionSyncVersion == FIRMWARE_VERSION) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (crossId.length() == 0 || crossId == "null") return;

  const int maxAttempts = 3;
  for (int attempt = 0; attempt < maxAttempts; attempt++) {
    if (updateCrossVersionInNocoDB()) {
      Serial.println("[NocoDB] Version sync cleared");
      return;
    }
    delay(500);
  }
  Serial.println("[NocoDB] Version sync still pending after retries");
}

void runBootPulse() {
  // Subtle boot pulse: brief low-intensity teal glow
  const uint8_t pulseSpeed = 25;
  const uint8_t maxBrightness = 50;

  // Teal (mix of green/blue, low intensity)
  for (uint8_t brightness = 0; brightness <= maxBrightness; brightness += 5) {
    uint8_t scaledValue = map(brightness, 0, maxBrightness, 0, 180);
    setLEDColor(0, scaledValue, scaledValue);
    delay(pulseSpeed);
  }

  delay(200);

  for (uint8_t brightness = maxBrightness; brightness > 0; brightness -= 5) {
    uint8_t scaledValue = map(brightness, 0, maxBrightness, 0, 180);
    setLEDColor(0, scaledValue, scaledValue);
    delay(pulseSpeed);
  }

  clearLEDs();
}

// ============================================================================
// NOCODB REGISTRATION (Cross record)
// ============================================================================

void registerCrossIfNeeded() {
  if (crossId.length() > 0 && crossId != "null") {
    if (verifyCrossIdExists()) {
      Serial.println("[NocoDB] Cross already registered id=" + crossId);
      return;
    }
    // Existing id not found on server; clear and re-register
    Serial.println("[NocoDB] Stored cross id missing on server, re-registering");
    crossId = "";
    preferences.putString("cross_id", "");
  }

  // Clear bad null values
  if (crossId == "null") {
    crossId = "";
    preferences.putString("cross_id", "");
  }

  if (fetchExistingCross()) {
    return;
  }

  createCrossRecord();
}

bool fetchExistingCross() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH);
  url += "?where=(device_key,eq," + DEVICE_KEY + ")";

  Serial.println("[NocoDB] Lookup URL: " + url);

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure(); // TODO: add root CA for production
  http.begin(client, url);
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }

  int code = http.GET();
  Serial.println("[NocoDB] Lookup response code: " + String(code));
  
  if (code != 200) {
    Serial.println("[NocoDB] Cross lookup failed code=" + String(code));
    String response = http.getString();
    Serial.println("[NocoDB] Response: " + response);
    http.end();
    return false;
  }

  String response = http.getString();
  Serial.println("[NocoDB] Lookup response: " + response.substring(0, 200));

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, response);
  http.end();
  if (err) {
    Serial.println("[NocoDB] Cross lookup JSON parse error: " + String(err.c_str()));
    return false;
  }

  JsonArray records = doc["list"];
  if (records.isNull()) {
    records = doc["data"].as<JsonArray>();
  }

  // Fallback lookup by device_name if device_key not present on server
  if (records.isNull() || records.size() == 0) {
    String urlByName = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH) + "?where=(device_name,eq," + DEVICE_NAME + ")";
    Serial.println("[NocoDB] Fallback lookup by name: " + urlByName);
    HTTPClient http2;
    WiFiClientSecure client2;
    client2.setInsecure();
    http2.begin(client2, urlByName);
    if (String(NOCODB_API_TOKEN).length() > 0) {
      http2.addHeader("xc-token", NOCODB_API_TOKEN);
    }
    int code2 = http2.GET();
    if (code2 == 200) {
      DynamicJsonDocument doc2(2048);
      DeserializationError err2 = deserializeJson(doc2, http2.getString());
      if (!err2) {
        records = doc2["list"];
        if (records.isNull()) {
          records = doc2["data"].as<JsonArray>();
        }
      }
    }
    http2.end();
  }

  if (records.isNull() || records.size() == 0) {
    Serial.println("[NocoDB] Cross not found, will create");
    return false;
  }

  JsonObject rec = records[0];
  String id = rec["Id"].as<String>();
  if (id.length() == 0) {
    Serial.println("[NocoDB] Cross lookup missing Id");
    return false;
  }

  crossId = id;
  preferences.putString("cross_id", crossId);
  if (!rec["intercession_count"].isNull()) {
    intercessionCount = rec["intercession_count"].as<long>();
    preferences.putLong("intercession_count", intercessionCount);
  }
  Serial.println("[NocoDB] Cross found id=" + crossId);
  return true;
}

bool updateCrossVersionInNocoDB() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (crossId.length() == 0 || crossId == "null") return false;

  String url = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH) + "?where=(Id,eq," + crossId + ")";

  StaticJsonDocument<256> doc;
  doc["Id"] = crossId;
  doc["firmware_version"] = FIRMWARE_VERSION;

  String payload;
  serializeJson(doc, payload);
  Serial.println("[NocoDB] Update version payload: " + payload);

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }

  int code = http.PATCH(payload);
  Serial.println("[NocoDB] Update version response code: " + String(code));

  bool success = (code >= 200 && code < 300);
  if (!success) {
    Serial.println("[NocoDB] Version update failed code=" + String(code));
    String response = http.getString();
    Serial.println("[NocoDB] Error: " + response);
  } else {
    Serial.println("[NocoDB] Version updated successfully");
    versionSyncPending = false;
    preferences.putBool("version_sync_pending", false);
    preferences.putString("version_sync_version", FIRMWARE_VERSION);
  }
  http.end();
  return success;
}

void updateCrossIntercessionCount() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (crossId.length() == 0 || crossId == "null") return;

  String url = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH) + "?where=(Id,eq," + crossId + ")";

  StaticJsonDocument<256> doc;
  doc["Id"] = crossId;
  doc["intercession_count"] = intercessionCount;

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }

  int code = http.PATCH(payload);
  if (code < 200 || code >= 300) {
    Serial.println("[NocoDB] Intercession update failed code=" + String(code));
  } else {
    Serial.println("[NocoDB] Intercession count updated to " + String(intercessionCount));
  }
  http.end();
}

void updateCrossHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (crossId.length() == 0 || crossId == "null") return;

  // Increment heartbeat counter for activity level
  heartbeatCount++;
  preferences.putLong("heartbeat_count", heartbeatCount);

  String url = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH) + "?where=(Id,eq," + crossId + ")";

  // Use RFC3339 format timestamp for compatibility
  time_t now = time(nullptr);
  struct tm* timeinfo = gmtime(&now);
  char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

  StaticJsonDocument<512> doc;
  doc["Id"] = crossId;
  doc["last_heartbeat"] = String(timestamp);
  doc["heartbeat_count"] = heartbeatCount;  // Sync activity level to NocoDB

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }

  int code = http.PATCH(payload);
  if (code >= 200 && code < 300) {
    Serial.println("[NocoDB] Heartbeat updated count=" + String(heartbeatCount));
  } else {
    Serial.println("[NocoDB] Heartbeat update failed code=" + String(code));
  }
  http.end();
}

bool createCrossRecord() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH);

  StaticJsonDocument<512> doc;
  doc["name"] = DEVICE_NAME;
  doc["device_name"] = DEVICE_NAME;
  doc["device_key"] = DEVICE_KEY;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["status"] = "active";
  doc["intercession_count"] = intercessionCount;

  String payload;
  serializeJson(doc, payload);
  Serial.println("[NocoDB] Create payload: " + payload);

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure(); // TODO: add root CA for production
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }

  int code = http.POST(payload);
  Serial.println("[NocoDB] Create response code: " + String(code));
  
  if (code < 200 || code >= 300) {
    Serial.println("[NocoDB] Cross create failed code=" + String(code));
    String response = http.getString();
    Serial.println("[NocoDB] Create error: " + response);
    // If conflict or error, attempt to fetch existing (handles duplicate device_key)
    if (code == 409 || code == 400) {
      Serial.println("[NocoDB] Retry fetch after create conflict");
      http.end();
      return fetchExistingCross();
    }
    http.end();
    return false;
  }

  String response = http.getString();
  Serial.println("[NocoDB] Create response: " + response.substring(0, 200));

  DynamicJsonDocument resp(2048);
  DeserializationError err = deserializeJson(resp, response);
  http.end();
  if (err) {
    Serial.println("[NocoDB] Cross create JSON parse error: " + String(err.c_str()));
    return false;
  }

  String id = resp["Id"].as<String>();
  if (id.length() == 0 && !resp["data"].isNull()) {
    id = resp["data"]["Id"].as<String>();
  }
  if (id.length() == 0) {
    JsonArray arr = resp["list"].as<JsonArray>();
    if (!arr.isNull() && arr.size() > 0) {
      id = arr[0]["Id"].as<String>();
    }
  }

  if (id.length() == 0) {
    Serial.println("[NocoDB] Cross create missing Id");
    return false;
  }

  crossId = id;
  preferences.putString("cross_id", crossId);
  Serial.println("[NocoDB] Cross created id=" + crossId);
  return true;
}

bool verifyCrossIdExists() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (crossId.length() == 0 || crossId == "null") return false;

  String url = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH) + "?where=(Id,eq," + crossId + ")";

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.begin(client, url);
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }

  int code = http.GET();
  if (code != 200) {
    Serial.println("[NocoDB] verify id failed code=" + String(code));
    http.end();
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, http.getString());
  http.end();
  if (err) return false;

  JsonArray records = doc["list"];
  if (records.isNull()) records = doc["data"].as<JsonArray>();
  if (records.isNull() || records.size() == 0) return false;
  return true;
}

// ============================================================================
// NOCODB POLLING (Prayers)
// ============================================================================

bool isCrossActive() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (crossId.length() == 0 || crossId == "null") return false;

  String url = String(NOCODB_BASE_URL) + String(NOCODB_CROSSES_PATH) + "?where=(Id,eq," + crossId + ")";

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.begin(client, url);
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }

  int code = http.GET();
  if (code != 200) {
    Serial.println("[NocoDB] Failed to check cross status code=" + String(code));
    http.end();
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, http.getString());
  http.end();

  if (err) {
    Serial.println("[NocoDB] Status check JSON parse error");
    return false;
  }

  JsonArray records = doc["list"];
  if (records.isNull()) {
    records = doc["data"].as<JsonArray>();
  }

  if (records.isNull() || records.size() == 0) {
    Serial.println("[NocoDB] Cross record not found");
    return false;
  }

  JsonObject rec = records[0];
  String status = rec["status"].as<String>();
  bool isActive = (status == "active");
  
  Serial.println("[NocoDB] Cross status: " + status + " (active=" + String(isActive) + ")");
  return isActive;
}

void pollPrayersIfDue() {
  unsigned long now = millis();
  if (now - lastNocoPollMs < NOCODB_POLL_MS) return;
  lastNocoPollMs = now;
  pollPrayers();
}

void checkForOTAUpdateIfDue() {
  unsigned long now = millis();
  if (now - lastOTACheckMs < OTA_CHECK_INTERVAL_MS) return;
  lastOTACheckMs = now;
  checkForUpdate();
}

void pollPrayers() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (crossId.length() == 0 || crossId == "null") return;

  // Check if this cross is active before processing prayers
  if (!isCrossActive()) {
    Serial.println("[NocoDB] Cross not active, skipping prayers");
    return;
  }

  // Update heartbeat
  updateCrossHeartbeat();

  String url = String(NOCODB_BASE_URL) + String(NOCODB_PRAYERS_PATH);
  // Filter by incremental Id to avoid date-format issues
  if (lastPrayerSeenId > 0) {
    url += "?where=(Id,gt," + String(lastPrayerSeenId) + ")&limit=20&sort=Id&order=asc";
  } else {
    url += "?limit=5&sort=Id&order=desc";
  }

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure(); // TODO: add root CA for production
  http.begin(client, url);
  if (String(NOCODB_API_TOKEN).length() > 0) {
    http.addHeader("xc-token", NOCODB_API_TOKEN);
  }
  int code = http.GET();
  if (code != 200) {
    Serial.println("[NocoDB] GET failed code=" + String(code));
    String resp = http.getString();
    Serial.println("[NocoDB] GET body: " + resp);
    http.end();
    return;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, http.getStream());
  if (err) {
    Serial.println("[NocoDB] JSON parse error");
    http.end();
    return;
  }
  http.end();

  JsonArray records = doc["list"];
  if (records.isNull()) {
    // Some NocoDB responses might use "data"; try fallback
    records = doc["data"].as<JsonArray>();
  }

  if (records.isNull() || records.size() == 0) {
    Serial.println("[NocoDB] No new prayers");
    return;
  }

  Serial.println("[NocoDB] Prayers fetched count=" + String(records.size()));
  String newestTs = lastPrayerSeenTs;
  long newestId = lastPrayerSeenId;
  int newPrayerCount = 0;

  for (JsonObject rec : records) {
    long recId = rec["Id"] | 0;
    if (recId > 0 && recId > lastPrayerSeenId) {
      newPrayerCount++;
      // Trigger LED pulse for each new prayer
      runPrayerPulse();
      if (newPrayerCount < records.size()) {
        delay(2000); // 2 second delay between prayer pulses
      }
    }
    if (recId > 0 && recId > newestId) {
      newestId = recId;
    }
    String created = rec["CreatedAt"] | "";
    if (created.length() > 0) {
      if (newestTs.length() == 0 || created > newestTs) {
        newestTs = created;
      }
    }
  }

  if (newPrayerCount > 0) {
    Serial.println("[NocoDB] New prayers: " + String(newPrayerCount));
    intercessionCount += newPrayerCount;
    preferences.putLong("intercession_count", intercessionCount);
    updateCrossIntercessionCount();
  }

  if (newestTs.length() > 0 && newestTs != lastPrayerSeenTs) {
    lastPrayerSeenTs = newestTs;
    preferences.putString("last_prayer_ts", lastPrayerSeenTs);
    Serial.println("[NocoDB] Updated last_prayer_ts=" + lastPrayerSeenTs);
  }
  if (newestId > lastPrayerSeenId) {
    lastPrayerSeenId = newestId;
    preferences.putLong("last_prayer_id", lastPrayerSeenId);
    Serial.println("[NocoDB] Updated last_prayer_id=" + String(lastPrayerSeenId));
  }
}

// ============================================================================
// API ENDPOINTS
// ============================================================================

void handleStatusAPI() {
  DynamicJsonDocument doc(256);
  doc["name"] = DEVICE_NAME;
  doc["version"] = FIRMWARE_VERSION;
  doc["mode"] = apModeActive ? "ap" : "station";
  doc["ip"] = apModeActive ? "192.168.4.1" : WiFi.localIP().toString();
  doc["wifi_saved"] = preferences.getString("wifi_ssid", "") != "";
  doc["intercession_count"] = intercessionCount;
  doc["heartbeat_count"] = heartbeatCount;

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

  delay(1000);
  ESP.restart();
}

void handleWiFiReset() {
  Serial.println("WiFi reset requested");
  
  preferences.putString("wifi_ssid", "");
  preferences.putString("wifi_password", "");
  
  server.send(200, "application/json", "{\"status\": \"reset\", \"message\": \"WiFi credentials cleared, rebooting...\"}");
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
// HTTP SERVER (shared AP/STA)
// ============================================================================

void startHttpServer(bool inAPMode) {
  if (serverStarted) return;

  if (inAPMode) {
    server.on("/", HTTP_GET, handlePortal);
    server.on("/save", HTTP_POST, handlePortalSave);
    server.onNotFound(handlePortal);
  }

  server.on("/api/status", HTTP_GET, handleStatusAPI);
  server.on("/api/scan-wifi", HTTP_GET, handleScanWiFiAPI);
  server.on("/api/configure-wifi", HTTP_POST, handleConfigureWiFiAPI);
  server.on("/api/reset-wifi", HTTP_POST, handleWiFiReset);
  server.on("/api/update", HTTP_POST, handleOTAUpdate);

  server.begin();
  serverStarted = true;
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

  http.begin(GITHUB_API_URL);
  http.addHeader("Accept", "application/vnd.github+json");
  int httpCode = http.GET();

  if (httpCode == 200) {
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, http.getStream());

    if (!err) {
      String latestVersion = doc["tag_name"].as<String>();
      latestVersion.replace("v", "");

      Serial.println("Latest version: " + latestVersion);

      if (latestVersion != String(FIRMWARE_VERSION)) {
        Serial.println("New version available, finding firmware...");

        JsonArray assets = doc["assets"];
        String downloadUrl = "";

        for (JsonObject asset : assets) {
          String assetName = asset["name"].as<String>();
          if (assetName == "cross.bin") {
            downloadUrl = asset["browser_download_url"].as<String>();
            break;
          }
        }

        if (downloadUrl.length() > 0) {
          Serial.println("Downloading from: " + downloadUrl);
          http.end();

          // Use WiFiClientSecure for HTTPS with proper timeout
          WiFiClientSecure client;
          client.setInsecure(); // TODO: add root CA for production
          client.setConnectionTimeout(15000); // 15 second connect timeout
          client.setTimeout(30000); // 30 second socket timeout

          // Ensure redirects are followed for GitHub asset URLs
          httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

          // Retry logic for download
          int maxRetries = 3;
          for (int attempt = 0; attempt < maxRetries; attempt++) {
            Serial.println("OTA download attempt " + String(attempt + 1) + "/" + String(maxRetries));
            
            t_httpUpdate_return ret = httpUpdate.update(client, downloadUrl);

            switch (ret) {
              case HTTP_UPDATE_FAILED:
                Serial.println("Update attempt " + String(attempt + 1) + " failed: " + String(httpUpdate.getLastErrorString()));
                if (attempt < maxRetries - 1) {
                  Serial.println("Retrying in 5 seconds...");
                  delay(5000);
                }
                break;
              case HTTP_UPDATE_NO_UPDATES:
                Serial.println("No update needed");
                return;
              case HTTP_UPDATE_OK:
                Serial.println("Update successful, rebooting...");
                // Mark version sync pending, ensure cross exists, then update DB before rebooting
                preferences.putBool("version_sync_pending", true);
                preferences.putString("version_sync_version", FIRMWARE_VERSION);
                registerCrossIfNeeded();
                updateCrossVersionInNocoDB();
                delay(1000);
                ESP.restart();
                break;
            }
            
            // Break on success or final failure
            if (ret != HTTP_UPDATE_FAILED || attempt == maxRetries - 1) {
              break;
            }
          }
        } else {
          Serial.println("Could not find cross.bin in release assets");
          http.end();
        }
      } else {
        Serial.println("Already on latest version");
        http.end();
      }
    } else {
      Serial.println("Failed to parse JSON response");
      http.end();
    }
  } else {
    Serial.println("Failed to check version: " + String(httpCode));
    http.end();
  }
}
