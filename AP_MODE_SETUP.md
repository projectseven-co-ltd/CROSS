# AP Mode Setup - Reverted from BLE

## Overview
Reverted from complex BLE approach to simple Access Point mode. Much simpler for users and works within Tauri's constraints.

## Device Setup Flow

### When Device Powers On (Unconfigured):
1. ESP32 boots with no saved WiFi credentials
2. Creates AP: `cross.{DEVICE_ID}` (e.g., `cross.1`)
3. AP IP: `192.168.4.1`
4. Runs REST API server with 3 endpoints

### When Device Powers On (Configured):
1. ESP32 boots with saved WiFi credentials
2. Connects to stored WiFi network
3. Advertises via mDNS as `{biblicalname}{3chars}.cross`
4. Tauri app can discover and connect

## User Flow

### Initial Setup:
1. User powers on unconfigured CROSS device
2. Device creates `cross.{ID}` WiFi hotspot
3. User connects to that hotspot with their phone/computer
4. Opens Tauri app
5. Clicks "Connect to Device"
6. App connects to `192.168.4.1` and shows available WiFi networks
7. User selects network and enters password
8. Device restarts and connects to main WiFi
9. Device now advertises via mDNS

### Later Discovery:
1. User opens Tauri app (now on same WiFi as device)
2. Clicks "Discover Devices"
3. App uses mDNS to find all `*.cross` devices on network
4. Shows list of discovered devices

## REST API Endpoints

All endpoints on `http://192.168.4.1`:

### GET /api/status
Returns device info while in AP mode.
```json
{
  "id": 1,
  "name": "luke3a7",
  "version": "3.2.0",
  "mode": "ap"
}
```

### GET /api/scan-wifi
Scans for available WiFi networks.
```json
[
  {
    "ssid": "MyNetwork",
    "encryption": "WPA2",
    "rssi": -65
  }
]
```

### POST /api/configure-wifi
Configures and saves WiFi credentials, then restarts device.
```json
{
  "ssid": "MyNetwork",
  "password": "secretpassword"
}
```

## Firmware
- **File**: `firmware/cross/cross.ino`
- **Size**: ~400 lines (includes WebServer + ArduinoJson)
- **Libraries**:
  - WiFi.h (built-in)
  - ESPmDNS.h (built-in)
  - Preferences.h (built-in, flash storage)
  - WebServer.h (built-in)
  - ArduinoJson.h (6.x)

## Tauri App
- **File**: `app/src/main.js`
- Three main flows:
  1. AP Mode Setup - Connect to `192.168.4.1` and configure WiFi
  2. Network Discovery - Use mDNS to find devices on main WiFi
  3. Submit Prayer - Send prayer text to discovered device

## Advantages
- No Web Bluetooth API limitations
- Simple, proven approach
- Works in all browsers/platforms
- Minimal additional libraries needed
- Users can see all available networks during setup
- Device can be configured without pre-existing WiFi
