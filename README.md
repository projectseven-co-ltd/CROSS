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