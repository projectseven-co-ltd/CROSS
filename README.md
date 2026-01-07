# CROSS - Community Response Prayer System

A complete system for placing physical prayer request devices (crosses) in communities, enabling real-time intercession through prayer submissions.

## Architecture

- **Firmware** (ESP32): WiFi provisioning + NocoDB polling
- **PWA App**: User registration + cross management + prayer submission  
- **NocoDB Backend**: Prayer database + cross registry
- **Desktop App** (Tauri): Advanced device setup and provisioning

## 🚀 Quick Start

### 1. Set up the PWA (Users interact here)
```bash
cd pwa-register
bash setup.sh  # or setup.bat on Windows
```

### 2. Configure NocoDB
Create tables in NocoDB - see [pwa-register/NOCODB_SETUP.md](pwa-register/NOCODB_SETUP.md)

### 3. Deploy PWA
Choose: Vercel, Netlify, GitHub Pages - see [pwa-register/DEPLOYMENT.html](pwa-register/DEPLOYMENT.html)

### 4. Generate & Print QR Codes
For physical crosses - see [pwa-register/QR_GENERATION.md](pwa-register/QR_GENERATION.md)

## 📚 Documentation

- 📱 [PWA App](pwa-register/README.md) - User registration & prayer submission
- 🗄️ [NocoDB Setup](pwa-register/NOCODB_SETUP.md) - Database schema  
- 🚀 [Deployment](pwa-register/DEPLOYMENT.html) - How to deploy PWA
- 📲 [QR Codes](pwa-register/QR_GENERATION.md) - Generate QR for crosses
- 📋 [Quick Reference](pwa-register/QUICK_REFERENCE.md) - Cheat sheet

## 🔌 Device HTTP API (Quick Reference)

- **GET /api/status** — device status (name, version, ip, saved wifi, email)
- **POST /api/configure-wifi** — configure WiFi via JSON body {"ssid":"...","password":"...","email":"optional@domain"}
- **POST /api/set-email** — set or update registered email for the device and attempt immediate sync to NocoDB

Examples:

- Set email via JSON (use curl.exe on PowerShell to avoid quoting issues):

```bash
curl.exe -X POST http://<device-host>/api/set-email -H "Content-Type: application/json" --data-raw '{"email":"you@example.com"}'
```

- Set email via form-encoded body (simple curl):

```bash
curl -X POST http://<device-host>/api/set-email -d "email=you@example.com"
```

- Configure WiFi + email via API:

```bash
curl.exe -X POST http://<device-host>/api/configure-wifi -H "Content-Type: application/json" --data-raw '{"ssid":"MyNet","password":"pass","email":"you@example.com"}'
```

Note: `/api/set-email` does **not** require an API token and is intended for local network use. If you want stricter control over device APIs, configure an API token via `/api/set-token` and enforce it in your workflows.

**Preference storage key**: the firmware stores the email in NVS under the key `reg_email` (short key chosen to avoid NVS key-length issues on some ESP32 devices). The device reads `reg_email` on boot and will sync it to NocoDB automatically.

If you are migrating from older firmware that used `registered_email`, it will still read from NocoDB on registration and re-populate `reg_email` when it pulls the record on boot.

## 🧪 Testing the API

Two small scripts are included to exercise `/api/set-email` and verify `/api/status`:

- `scripts/test_set_email.sh` (bash)
- `scripts/test_set_email.ps1` (PowerShell)

Examples:

- Test without auth:

```bash
./scripts/test_set_email.sh http://jacob-f4e9d4.local you@example.com
```

- Test with auth (set token first):

```bash
# Set token (use curl.exe on PowerShell if needed)
curl.exe -X POST http://<device-host>/api/set-token -H "Content-Type: application/json" --data-raw '{"token":"s3cr3t"}'

# Run test helper (bash)
./scripts/test_set_email.sh http://<device-host> s3cr3t you@example.com

# Or PowerShell
.
\scripts\test_set_email.ps1 -Device 'http://<device-host>' -Token 's3cr3t' -Email 'you@example.com'
```

The scripts will print the response and the `/api/status` payload to help confirm success.

## 🎯 Features

- User registration and account management
- Cross device registration via QR code or device name
- Prayer submission for intercession
- Real-time prayer polling from NocoDB
- LED indicators for prayer notifications
- Offline support (PWA service worker)
- Activity tracking and prayer counts
- Geographic area assignment

## 🛠 Technology Stack

- **Frontend**: HTML5, CSS3, Vanilla JavaScript (no build tools needed)
- **Backend**: NocoDB (no-code database)
- **Hardware**: ESP32 with custom firmware
- **Desktop**: Tauri app for device provisioning
- **Hosting**: Any static web host (Vercel, Netlify, Azure, etc)

## 📊 Workflow

```
1. User creates account (PWA)
   ↓
2. Register CROSS device (scan QR code)
   ↓
3. User submits prayer (PWA)
   ↓
4. Prayer stored in NocoDB
   ↓
5. Device polls NocoDB every 60s
   ↓
6. Device LED pulses on new prayer
   ↓
7. Prayer count updates in database
```

## ⚙️ Hardware Pinout

### ESP32-WROOM
ESP-32S Board ESP-WROOM-32

**RGB LED Strip (Non-addressable, Common Anode):**
- GPIO 16: Red channel
- GPIO 17: Green channel
- GPIO 18: Blue channel

PWM Configuration:
- Frequency: 5000 Hz
- Resolution: 8-bit (0-255)
- Common anode: 255 = OFF, 0 = ON