# ESP32 Nano WiFi Setup Guide: Captive Portal + mDNS

This guide explains how to set up WiFi onboarding on the ESP32 Nano using a captive portal and mDNS for easy discovery and configuration.

---

## Overview

The ESP32 will:
1. Start in **Access Point (AP) mode** with SSID `cross.{cross_id}` (e.g., `cross.1`).
2. Host a **captive portal** for users to enter WiFi credentials.
3. Advertise itself via **mDNS** at `cross.{cross_id}.local`.
4. Transition to **Station (STA) mode** once connected to WiFi.
5. Store credentials in **EEPROM** for persistent reconnection.

---

## Required Libraries

Install via Arduino IDE Library Manager:
- **WiFi** (built-in for ESP32)
- **DNSServer** (built-in)
- **WebServer** (built-in)
- **ESPmDNS** (built-in)
- **EEPROM** (built-in)
- **ArduinoJson** (for config storage, optional)

---

## Hardware Setup

### Connections
- ESP32 Nano to power supply (3.3V from TP4056 charger module)
- No additional WiFi hardware needed (built-in radio)

---

## Firmware Configuration

### 1. Set Cross ID
Define a unique cross ID (hardcoded or stored in EEPROM):
```cpp
const uint16_t CROSS_ID = 1;  // Unique identifier for this cross
String SSID_PREFIX = "cross.";
String AP_SSID = SSID_PREFIX + String(CROSS_ID);  // e.g., "cross.1"
```

### 2. Captive Portal Setup
- AP runs on IP `192.168.4.1`
- DNS server redirects all `.local` queries to the AP
- Web server listens on port `80`
- HTML form accepts SSID and password

### 3. mDNS Advertising
- Hostname: `cross-{cross_id}` (e.g., `cross-1`)
- Full mDNS address: `cross-1.local`
- Accessible from any device on the network after WiFi connection

### 4. Credential Storage
- EEPROM stores last successful WiFi SSID and hashed/encoded password
- On boot, ESP32 attempts to reconnect using stored credentials
- If connection fails, falls back to AP mode

---

## Setup Flow

```
[Boot]
   ↓
[Check stored WiFi creds in EEPROM]
   ↓
[Yes] → [Try to connect to WiFi (STA mode)]
   ├─ Success → [mDNS advertises cross-{id}.local, poll API]
   └─ Fail → [Fall back to AP + Captive Portal]
   ↓
[No stored creds] → [Start AP mode + Captive Portal]
   ↓
[User connects to "cross.{id}" and navigates to any domain]
   ↓
[DNS redirects to 192.168.4.1 (captive portal page)]
   ↓
[User enters WiFi SSID and password]
   ↓
[Form posts to /api/setup]
   ↓
[ESP32 stores credentials, switches to STA mode]
   ↓
[Success → Advertise mDNS, begin polling API]
```

---

## Testing

### 1. Connect to the Cross AP
- Look for SSID `cross.1` (or your cross ID)
- Connect without password (open AP)

### 2. Open Captive Portal
- Most devices auto-open a captive portal page
- Manually navigate to `http://192.168.4.1` or `http://cross.1.local`

### 3. Enter WiFi Credentials
- Select or enter your WiFi network SSID
- Enter password
- Click "Connect"

### 4. Verify mDNS
After WiFi connection, ping from another device:
```bash
ping cross-1.local
# or
ping cross.1.local  # depending on hostname format
```

### 5. Check API Polling
Verify the device begins polling your n8n API endpoint for status updates.

---

## Security Notes

- Captive portal is **open (no password)** for ease of setup. Consider adding WPA2 if needed.
- Store WiFi passwords encrypted in EEPROM (use ArduinoJson + simple XOR or AES).
- Implement a **reset button** to clear stored credentials and restart setup.

---

## Future Enhancements

- [ ] Over-the-air (OTA) firmware updates
- [ ] Password-protected captive portal
- [ ] QR code for faster setup (encode WiFi credentials)
- [ ] Remote configuration via API endpoint
- [ ] Bluetooth fallback for setup if WiFi unavailable

---

## References

- [Arduino ESP32 WiFi Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [mDNS/Bonjour Service Discovery](https://en.wikipedia.org/wiki/Multicast_DNS)
- [Captive Portal Pattern](https://en.wikipedia.org/wiki/Captive_portal)
