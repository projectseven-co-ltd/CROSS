# WiFi Setup Guide

## Overview

The Tauri app now includes complete WiFi configuration capabilities for ESP32 crosses. This eliminates the need for a web interface on the ESP32, saving flash space and simplifying the firmware.

## Architecture

### ESP32 Firmware (API-Only)
- **File**: `/firmware/cross/cross_api.ino`
- **Approach**: REST API endpoints only, no web interface
- **WiFi Modes**:
  - **AP Mode**: When no WiFi configured, creates network `cross.{ID}` at `192.168.4.1`
  - **Station Mode**: After WiFi configured, connects to network and advertises via mDNS

### Tauri App Features
1. **Network Discovery**: Finds crosses on local network via mDNS
2. **Direct Connection**: Connect to cross by ID (e.g., `luke3a7.cross`)
3. **WiFi Scanning**: Scan for available WiFi networks from cross
4. **WiFi Configuration**: Set WiFi credentials and connect cross to network

## WiFi Setup Workflow

### Initial Setup (No WiFi)
1. ESP32 creates AP mode network: `cross.{ID}`
2. User connects device to this AP
3. Tauri app connects to cross at `192.168.4.1`
4. User scans for WiFi networks
5. User selects network and enters password
6. Cross saves credentials and reboots
7. Cross connects to WiFi and advertises via mDNS as `{biblicalname}{3chars}.cross`

### After Setup
1. Cross is discoverable on network via mDNS
2. App can find cross automatically
3. WiFi can be reconfigured if needed

## API Endpoints

### GET /status
Returns cross status including:
- Firmware version
- Device name
- WiFi status
- IP address
- Signal strength
- Memory and uptime

```json
{
  "firmware_version": "1.0.0",
  "cross_id": 1,
  "geo_id": 1,
  "device_name": "luke3a7",
  "hostname": "luke3a7.cross",
  "wifi_connected": true,
  "wifi_ssid": "MyNetwork",
  "ip_address": "192.168.1.100",
  "rssi": -45,
  "free_heap": 234567,
  "uptime": 12345
}
```

### GET /wifi/scan
Scans for available WiFi networks:

```json
{
  "networks": [
    {
      "ssid": "MyNetwork",
      "rssi": -45,
      "encryption": true
    },
    {
      "ssid": "OpenNetwork",
      "rssi": -67,
      "encryption": false
    }
  ]
}
```

### POST /wifi
Configures WiFi credentials:

**Request:**
```json
{
  "ssid": "MyNetwork",
  "password": "mypassword"
}
```

**Response:**
```json
{
  "success": true,
  "message": "WiFi configured. Restarting..."
}
```

### POST /prayer
Submit prayer data:

**Request:**
```json
{
  "prayer_id": 123,
  "geo_id": 1,
  "timestamp": 1234567890
}
```

**Response:**
```json
{
  "success": true,
  "forwarded": true
}
```

### POST /reset
Reset cross to factory defaults (clears WiFi):

```json
{
  "success": true,
  "message": "Resetting to factory defaults..."
}
```

## Tauri App Functions

### Rust Backend (`src-tauri/src/main.rs`)

- `discover_crosses(timeout_seconds)` - mDNS network discovery
- `resolve_cross_direct(cross_id)` - Direct hostname resolution
- `test_connection(hostname)` - Connection testing
- `get_cross_status(ip_address)` - Fetch cross status
- `scan_wifi_networks(ip_address)` - Scan WiFi networks
- `configure_wifi(ip_address, ssid, password)` - Set WiFi credentials
- `send_prayer(ip_address, prayer_data)` - Submit prayer

### Frontend (`index.html`, `main.js`)

- `handleDiscovery()` - Scan network for crosses
- `handleDirectConnect()` - Connect to specific cross
- `handleWiFiScan()` - Scan for WiFi networks
- `selectNetwork(ssid, requiresPassword)` - Select WiFi network
- `handleWiFiConfigure()` - Configure WiFi on cross
- `showWiFiSetup(crossId, ipAddress)` - Show WiFi setup UI

## Biblical Naming

Crosses use biblical names with 3 random characters:
- 24 biblical names: Luke, John, Mark, Matthew, Paul, Peter, etc.
- 3 random alphanumeric characters: a-z, 0-9
- Example: `luke3a7.cross`

Generated in ESP32 firmware on first boot and stored in EEPROM/Preferences.

## UI Design

Black and white minimalist design as requested:
- High contrast for readability
- Simple, clean layout
- No emojis (per user requirement)
- Clear status indicators
- Network signal strength bars

## Testing

### Test mDNS Discovery
```bash
dns-sd -B _http._tcp
```

### Test API Endpoints
```bash
# Get status
curl http://192.168.4.1:80/status

# Scan WiFi
curl http://192.168.4.1:80/wifi/scan

# Configure WiFi
curl -X POST http://192.168.4.1:80/wifi \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MyNetwork","password":"mypassword"}'
```

## Future Enhancements

1. **Prayer Management**: UI for viewing and managing prayers
2. **Cross Monitoring**: Dashboard showing all crosses on network
3. **Firmware Updates**: OTA update capability via app
4. **Analytics**: Prayer statistics and cross health monitoring
5. **Multi-cross Management**: Configure multiple crosses at once

## Benefits of API-Only Approach

1. **Flash Space**: Saves ~100KB by removing HTML/CSS/JS from ESP32
2. **Centralized UI**: All UI logic in one place (Tauri app)
3. **Better UX**: Native app experience vs web interface
4. **Consistency**: Same UI across all platforms (desktop/mobile)
5. **Easier Updates**: Update app without touching firmware
6. **Offline Capable**: App works without internet connection
