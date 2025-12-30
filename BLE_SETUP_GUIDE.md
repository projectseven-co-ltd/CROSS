# Bluetooth Low Energy (BLE) Setup Guide

## Overview

The CROSS system now uses **Bluetooth Low Energy (BLE)** for initial WiFi configuration, making setup easier and more reliable than AP mode.

## Hardware & Libraries

### ESP32 Firmware
- **Library**: NimBLE (already included in ESP32 core)
- **Baud Rate**: 115200
- **BLE Device Name**: `CROSS-{ID}` (e.g., `CROSS-1`)

### Tauri App
- **Web Bluetooth API** (built into Chrome, Edge, Opera, and Chromium-based browsers)
- Works on desktop and Android devices with Bluetooth support

## Setup Workflow

### Device Startup
1. ESP32 boots and loads saved configuration
2. Starts BLE server advertising as `CROSS-{ID}`
3. Waits for Bluetooth connection from Tauri app
4. Once WiFi credentials received, restarts and connects to WiFi
5. After WiFi connected, device advertises via mDNS as `{biblicalname}{3chars}.cross`

### User Setup Steps
1. Open Tauri app on phone/desktop (Chrome, Edge, or Chromium-based browser)
2. Click **"Scan for Crosses"** under "Initial Setup - Bluetooth Pairing"
3. Select `CROSS-{ID}` from the device picker
4. Enter WiFi SSID and password
5. Click **"Send WiFi Credentials"**
6. Device will restart and connect to WiFi
7. Once online, device appears in "Discover Crosses on Network" section

## BLE Service Structure

### Service UUID
```
12345678-1234-1234-1234-123456789012
```

### Characteristics

#### WiFi SSID (Write)
- **UUID**: `12345678-1234-1234-1234-123456789013`
- **Permission**: Write
- **Description**: Send WiFi network SSID
- **Example**: `"MyNetwork"`

#### WiFi Password (Write)
- **UUID**: `12345678-1234-1234-1234-123456789014`
- **Permission**: Write
- **Description**: Send WiFi password
- **Example**: `"mypassword123"`

#### Status (Read/Notify)
- **UUID**: `12345678-1234-1234-1234-123456789015`
- **Permission**: Read, Notify
- **Description**: Receive device status updates
- **Examples**:
  - `"Waiting for WiFi..."`
  - `"SSID saved"`
  - `"WiFi configured, restarting..."`

## Firmware Implementation

### BLE Server Setup
```cpp
NimBLEDevice::init("CROSS-" + String(CROSS_ID));
NimBLEServer* pServer = NimBLEDevice::createServer();
// Create service and characteristics...
NimBLEDevice::getAdvertising()->start();
```

### Characteristic Callbacks
- **SSID Write**: Stores SSID in Preferences
- **Password Write**: Stores password, triggers restart
- **Status Notify**: Sends device status updates to client

## Tauri App Implementation

### Web Bluetooth API Usage
```javascript
const device = await navigator.bluetooth.requestDevice({
  filters: [{ namePrefix: 'CROSS-' }],
  optionalServices: ['12345678-1234-1234-1234-123456789012']
});

const server = await device.gatt.connect();
const service = await server.getPrimaryService('12345678-1234-1234-1234-123456789012');
```

### Sending Credentials
```javascript
await characteristic.writeValue(new TextEncoder().encode(value));
```

### Receiving Status
```javascript
await characteristic.startNotifications();
characteristic.addEventListener('characteristicvaluechanged', (event) => {
  const status = new TextDecoder().decode(event.target.value);
  console.log('Status:', status);
});
```

## Browser Compatibility

### Supported
- ✅ Chrome 56+
- ✅ Edge 79+
- ✅ Opera 43+
- ✅ Android Chrome
- ✅ Android Edge

### Not Supported
- ❌ Safari (iOS/macOS) - Use alternative app
- ❌ Firefox - Use alternative app

## Security Considerations

### Current Implementation
- BLE pairing not enforced (device is in pairing mode)
- No encryption on WiFi credentials transfer
- Device name includes ID (CROSS-1, etc.)

### Future Enhancements
1. **BLE Pairing**: Require pin code for connection
2. **Encryption**: Use HTTPS characteristics or BLE encryption
3. **Timeout**: Auto-disable BLE after WiFi configured or time elapsed
4. **Authentication**: Add device PIN in preferences

## Troubleshooting

### "Bluetooth not supported"
- Use Chrome, Edge, or Opera browser
- On desktop: Enable Bluetooth in system settings
- On Android: Enable Bluetooth permission for app

### Device not found in scan
1. Check if ESP32 is powered on
2. Verify BLE is working: Monitor serial output should show `[BLE] Server started`
3. Restart ESP32 (power cycle)
4. Check browser console for errors

### "Failed to connect to device"
1. Try scanning again
2. Restart app and ESP32
3. Check if another app is blocking BLE access

### Credentials sent but device doesn't connect
1. Check WiFi SSID spelling (case-sensitive)
2. Verify password is correct
3. Check if network is 2.4GHz (some networks only broadcast 5GHz)
4. Check ESP32 serial output for connection logs

### Device not appearing on network after BLE setup
1. Wait 10-15 seconds for device to restart and connect
2. Try "Scan Network" after waiting
3. Check router's client list to see if device connected
4. Verify WiFi network is working from other devices

## Technical Details

### Power States
- **BLE Advertising**: ~25mW (low power)
- **BLE Connected**: ~35mW (low power)
- **WiFi Scanning**: ~200mW (medium power)
- **WiFi Connected**: ~150-200mW (medium power)

### Memory Usage
- BLE Server: ~50KB
- HTTP Server (WiFi mode): ~30KB
- mDNS: ~20KB
- Preferences/EEPROM: ~4KB

### Timeline
1. **Boot**: 2-3 seconds
2. **BLE Advertising**: Immediate
3. **WiFi Connection**: 5-15 seconds (depends on signal)
4. **mDNS Registration**: 1-2 seconds after WiFi

## Fallback Options

If BLE setup fails:
1. **Direct Connection**: Use "Direct Connection" with Cross ID in app
2. **Manual AP Mode**: (Future) Could add button to enable AP mode for manual setup

## Testing

### Desktop Testing
```bash
# View BLE devices (macOS)
system_profiler SPBluetoothDataType

# Scan via nRF Connect app (cross-platform)
# Download from Play Store or App Store
```

### ESP32 Serial Monitor
```
[BLE] Server started, advertising as: CROSS-1
[BLE] Client connected
[BLE] SSID received: MyNetwork
[BLE] Password received
[WiFi] Connecting to: MyNetwork
[WiFi] Connected!
[WiFi] IP: 192.168.1.100
[mDNS] Started: john2f5.cross
```

## Future Enhancements

1. **Cross Platform**: Electron app for Windows support
2. **CLI Tool**: Command-line WiFi configuration
3. **Multiple Devices**: Setup multiple crosses in one session
4. **OTA Updates**: Firmware updates via BLE
5. **Factory Reset**: Long press button to trigger reset mode
6. **Manual Pairing**: PIN code for manual device selection
