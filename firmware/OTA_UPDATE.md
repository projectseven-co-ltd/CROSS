# OTA Update Setup

## Overview
Added Over-The-Air (OTA) firmware update capability from GitHub.

## Setup Steps

### 1. Update GitHub URLs
In [firmware/cross/cross.ino](firmware/cross/cross.ino), replace `YOUR_USERNAME` with your GitHub username:

```cpp
const char* GITHUB_FIRMWARE_URL = "https://raw.githubusercontent.com/YOUR_USERNAME/CROSS/main/firmware/cross.bin";
const char* GITHUB_VERSION_URL = "https://raw.githubusercontent.com/YOUR_USERNAME/CROSS/main/firmware/version.txt";
```

### 2. Build and Upload Firmware Binary
After compiling in Arduino IDE:
1. Go to `Sketch > Export Compiled Binary`
2. This creates `cross.ino.bin` in the firmware folder
3. Rename it to `cross.bin`
4. Commit and push to GitHub

### 3. Update Version
When releasing new firmware:
1. Update `FIRMWARE_VERSION` in the code (e.g., `"3.4.0"`)
2. Update `firmware/version.txt` to match
3. Build and export new `cross.bin`
4. Commit all changes to GitHub

## Usage

### Trigger Update via API
```bash
curl -X POST http://192.168.4.1/api/update
# or
curl -X POST http://luke3a7.local/api/update
```

### How It Works
1. Device checks `version.txt` on GitHub
2. Compares with current `FIRMWARE_VERSION`
3. If newer version available, downloads `cross.bin`
4. Installs firmware and reboots
5. Device comes back online with new version

## Requirements
- Device must be connected to WiFi (not in AP mode)
- GitHub repository must be public (or use auth tokens)
- Firmware binary must be committed to repo

## Security Note
Update `GITHUB_FIRMWARE_URL` to use your actual repository URL. Consider:
- Using GitHub releases instead of raw files
- Adding authentication for private repos
- Implementing signature verification for production
