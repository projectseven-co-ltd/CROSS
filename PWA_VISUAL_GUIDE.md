# CROSS Registration PWA - Visual Overview

## 🎯 System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     CROSS SYSTEM                            │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  USERS (Web/Mobile)              PHYSICAL DEVICES           │
│  ┌──────────────────┐            ┌──────────────┐           │
│  │   PWA App        │            │  ESP32 Cross │           │
│  │  (pwa-register)  │            │   w/ LEDs    │           │
│  │                  │            │              │           │
│  │ • Register       │            │ • WiFi ready │           │
│  │ • Login          │            │ • QR attached│           │
│  │ • Manage crosses │            │ • Polls DB   │           │
│  │ • Submit prayers │            │ • LED pulse  │           │
│  └────────┬─────────┘            └───────┬──────┘           │
│           │                               │                  │
│           └───────────────┬───────────────┘                  │
│                           │                                  │
│                      HTTPS API                              │
│                           │                                  │
│           ┌───────────────▼───────────────┐                 │
│           │      NocoDB Database         │                 │
│           │  (noco.app.p7n.net)         │                 │
│           │                              │                 │
│           │ • Users table               │                 │
│           │ • Crosses table             │                 │
│           │ • Prayers table             │                 │
│           │ • Geo areas table           │                 │
│           └──────────────────────────────┘                 │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## 📱 PWA App Flow

```
Visit PWA URL
    │
    ▼
┌─────────────────┐
│  Auth Screen    │
│  ┌───────────┐  │
│  │ Login     │  │
│  │ Sign Up   │  │
│  └───────────┘  │
└────────┬────────┘
         │
         ▼
   ┌──────────────┐
   │ Check creds  │
   │ in NocoDB    │
   └──────┬───────┘
          │
          ▼
   ┌──────────────────┐
   │   Home Screen    │
   │                  │
   │ • Register Cross │◄─────────────┐
   │ • Submit Prayer  │              │
   │ • My Crosses     │              │
   │ • Sign Out       │              │
   └────────┬─────────┘              │
            │                        │
            ├─────────────────────┐  │
            │                     │  │
            ▼                     ▼  │
    ┌────────────────┐      ┌────────────┐
    │ Register Cross │      │My Crosses  │
    │                │      │            │
    │ • Scan QR      │      │• View list │
    │ • Enter name   │      │• See stats │
    │ • Area select  │      │• QR export │
    └────────┬───────┘      └────────────┘
             │
             ▼
      Create/Update
      in NocoDB
             │
             └──────────────────────┘
```

## ⚙️ Device Registration Flow

```
Physical Cross
    │
    ├─ QR Code with device_key
    │  (e.g., A1B2C3D4E5F6)
    │
    └─► User scans with phone
         │
         ▼
      PWA opens
         │
         ▼
      Device name auto-fills
      (from QR data)
         │
         ▼
      User selects geo_area
         │
         ▼
      Submit registration
         │
         ▼
      NocoDB crosses table
      updated with:
      • user_id
      • geo_area
      • status: "active"
         │
         ▼
      ESP32 polls NocoDB
      and sees user_id set
         │
         ▼
      Cross now active!
```

## 🙏 Prayer Flow

```
User submits prayer
(in PWA app)
    │
    ├─ Select cross
    ├─ Write prayer text
    └─ Submit
         │
         ▼
   POST to NocoDB
   prayers table:
   • prayer_text
   • cross_id
   • user_id
   • created_at
         │
         ▼
   ┌──────────────────────┐
   │ Every 60 seconds:    │
   │ ESP32 polls NocoDB   │
   │ for new prayers      │
   └──────────────────────┘
         │
         ▼
   New prayer found!
         │
         ├─ intercession_count++
         ├─ LED pulses CYAN
         └─ Updates NocoDB
                │
                ▼
           Prayer logged
```

## 📊 Data Model

```
┌────────────────────────────────────────────────────┐
│ USERS                                              │
├────────────┬──────────────┬──────────────────────┤
│ id (PK)    │ name         │ email                │
├────────────┼──────────────┼──────────────────────┤
│ 1          │ John Doe     │ john@example.com     │
│ 2          │ Jane Smith   │ jane@example.com     │
└────────────────────────────────────────────────────┘
        │
        │ user_id (FK)
        │
        ▼
┌────────────────────────────────────────────────────┐
│ CROSSES                                            │
├────────────┬──────────────┬──────────────────────┤
│ id (PK)    │ device_name  │ device_key           │
├────────────┼──────────────┼──────────────────────┤
│ 1          │ luke-3A7     │ A1B2C3D4E5F6        │
│ 2          │ john-F2C5    │ F2C5D6E7F8A9        │
└────────────────────────────────────────────────────┘
        │
        │ cross_id (FK)
        │
        ▼
┌────────────────────────────────────────────────────┐
│ PRAYERS                                            │
├────────────┬──────────────┬──────────────────────┤
│ id (PK)    │ prayer_text  │ cross_id             │
├────────────┼──────────────┼──────────────────────┤
│ 1          │ "Please..." │ 1 (luke-3A7)         │
│ 2          │ "Help with"│ 2 (john-F2C5)        │
└────────────────────────────────────────────────────┘
```

## 🚀 Deployment Options

```
┌──────────────────────────────────────────────────┐
│           Your PWA App                           │
│      (pwa-register folder)                       │
└────────────┬─────────────────────────────────────┘
             │
    ┌────────┴─────────┬────────────┬──────────┐
    │                  │            │          │
    ▼                  ▼            ▼          ▼
 Vercel            Netlify    GitHub Pages   Azure
 (1 min)           (2 min)     (5 min)    (5 min)
 ★ Best            Drag&      Free        Enterprise
 Free              Drop       GitHub       Integration
 Auto-deploy       Free       Free


All point to: https://your-domain.com/pwa-register
```

## 🔄 Update Cycle (Device)

```
ESP32 Boot
    │
    ├─ Load WiFi settings
    ├─ Connect to network
    ├─ Register in NocoDB
    └─ Start polling
         │
         └─► Every 60 seconds:
             │
             ├─ Poll NocoDB
             │  for new prayers
             │
             ├─ Compare last_id
             │  with new prayers
             │
             ├─ LED pulse CYAN
             │  for each prayer
             │
             ├─ Update counter
             │
             └─ Log activity
```

## 📱 Mobile App Installation

```
┌──────────────────────────────┐
│       ANDROID (Chrome)       │
├──────────────────────────────┤
│                              │
│ 1. Open PWA in Chrome        │
│ 2. Menu (⋮) →  Install app  │
│ 3. App appears on home       │
│                              │
│    Takes 30 seconds          │
└──────────────────────────────┘

┌──────────────────────────────┐
│      iPhone/iPad (Safari)    │
├──────────────────────────────┤
│                              │
│ 1. Open PWA in Safari        │
│ 2. Share → Add to Home       │
│    Screen                    │
│ 3. Icon appears on home      │
│                              │
│    Takes 20 seconds          │
└──────────────────────────────┘

┌──────────────────────────────┐
│    Desktop (Chrome/Edge)     │
├──────────────────────────────┤
│                              │
│ 1. Click install icon in     │
│    address bar               │
│ 2. App opens in window       │
│ 3. Like native app           │
│                              │
│    Takes 10 seconds          │
└──────────────────────────────┘
```

## 🔐 Security Layers

```
User → HTTPS → PWA App → HTTPS API → NocoDB DB
  ↓       ↓        ↓          ↓           ↓
Auth   Encrypted  Offline    Secure    Encrypted
Token   Transit   Cache      Token      Storage
```

## 📁 File Structure & Purpose

```
pwa-register/
│
├── index.html ...................... Main UI
│   └─ Screens: Auth, Home, Register, Prayer
│
├── app.js .......................... Logic
│   ├─ Login/Signup handlers
│   ├─ Cross registration
│   ├─ Prayer submission
│   └─ NocoDB API calls
│
├── sw.js ........................... Service Worker
│   ├─ Offline support
│   ├─ Cache strategy
│   └─ Background sync
│
├── manifest.json ................... PWA metadata
│   ├─ App name & icons
│   ├─ Shortcuts
│   └─ Display settings
│
├── README.md ....................... Main docs
├── QUICK_REFERENCE.md .............. Cheat sheet
├── DEPLOYMENT.html ................. Deploy guide
├── NOCODB_SETUP.md ................. DB config
├── QR_GENERATION.md ................ QR codes
│
├── setup.sh ........................ Linux/Mac setup
└── setup.bat ....................... Windows setup
```

## ✅ Workflow Summary

```
START
  │
  ├─► User creates account (PWA)
  │        └─► Stored in users table
  │
  ├─► Physical cross installed
  │        └─► Has QR code with device_key
  │
  ├─► User scans QR code
  │        └─► Opens PWA app
  │
  ├─► User registers cross
  │        └─► Updates crosses table with user_id
  │
  ├─► ESP32 detects user_id set
  │        └─► Cross now "active"
  │
  ├─► User submits prayer
  │        └─► Prayer stored in NocoDB
  │
  ├─► ESP32 polls every 60 seconds
  │        └─► Finds new prayer
  │
  ├─► LED pulses CYAN
  │        └─► Visual feedback
  │
  ├─► Counter incremented
  │        └─► Activity tracked
  │
  └─► REPEAT
       (Users submit prayers, device responds)
```

## 🎯 Next Action Items

```
[ ] 1. Read DEPLOYMENT.html
[ ] 2. Set up NocoDB tables
[ ] 3. Update app.js CONFIG
[ ] 4. Test locally
[ ] 5. Deploy to Vercel/Netlify
[ ] 6. Generate QR codes
[ ] 7. Print QR codes
[ ] 8. Install on physical crosses
[ ] 9. Share URL with community
[ ] 10. Monitor prayer submissions
```

---

**You now have a complete, production-ready PWA for CROSS registration!** ✝️

Each file in `pwa-register/` has detailed documentation. Start with DEPLOYMENT.html for step-by-step instructions.
