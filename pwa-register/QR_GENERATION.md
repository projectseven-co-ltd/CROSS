# QR Code Generation for Physical Crosses

This guide explains how to generate and print QR codes for your CROSS devices.

## What's in the QR Code?

Each QR code encodes:
```json
{
  "device_key": "A1B2C3D4E5F6",
  "register_url": "https://your-pwa-domain.com/pwa-register"
}
```

When scanned, users can quickly access the registration app.

## Method 1: Generate via PWA (Easy)

1. Deploy the PWA and log in
2. Go to "My Crosses"
3. Click "View QR Code" on any cross
4. A modal shows the QR code
5. Screenshot and print it

## Method 2: Generate via Command Line

### Using Node.js

```bash
npm install -g qrcode

# Generate QR code for a device
qrcode '{"device_key":"A1B2C3D4E5F6","register_url":"https://your-pwa.com/pwa-register"}' -o qr-luke-3a7.png
```

### Using Python

```bash
pip install qrcode[pil]

python3 << 'EOF'
import qrcode
import json

device_key = "A1B2C3D4E5F6"  # From your cross
register_url = "https://your-pwa.com/pwa-register"

data = json.dumps({
    "device_key": device_key,
    "register_url": register_url
})

qr = qrcode.QRCode(version=1, box_size=10, border=5)
qr.add_data(data)
qr.make(fit=True)

img = qr.make_image(fill_color="black", back_color="white")
img.save(f"qr-{device_key}.png")
EOF
```

## Method 3: Online QR Code Generator

1. Go to https://www.qr-code-generator.com/
2. Paste the JSON data:
```json
{"device_key":"A1B2C3D4E5F6","register_url":"https://your-pwa.com/pwa-register"}
```
3. Download as PNG
4. Print!

## Method 4: Batch Generate for All Devices

### Using the API to get all device keys:

```bash
curl -H "xc-token: YOUR_TOKEN" \
  "https://noco.app.p7n.net/api/v2/tables/mdk4garchpkpvsd/records" \
  | jq '.list[] | {device_name, device_key}'
```

Then use Node.js or Python to batch generate:

```python
import qrcode
import json
import requests

# Get all crosses from NocoDB
headers = {"xc-token": "YOUR_TOKEN"}
response = requests.get(
    "https://noco.app.p7n.net/api/v2/tables/mdk4garchpkpvsd/records",
    headers=headers
)

crosses = response.json()['list']
register_url = "https://your-pwa.com/pwa-register"

for cross in crosses:
    device_key = cross['device_key']
    device_name = cross.get('device_name', device_key)
    
    data = json.dumps({
        "device_key": device_key,
        "register_url": register_url
    })
    
    qr = qrcode.QRCode(version=1, box_size=10, border=5)
    qr.add_data(data)
    qr.make(fit=True)
    
    img = qr.make_image(fill_color="black", back_color="white")
    img.save(f"qr-{device_name}.png")
    
    print(f"Generated QR for {device_name}")
```

## Printing and Installation

### Label Size Recommendations

- **Small labels** (2"×2"): For attachment to cross frame
- **Medium labels** (3"×3"): For standalone QR cards
- **Large labels** (4"×4"): For posters or signage

### Printing on Labels

1. Generate PNG QR codes
2. Use label printing software (Avery, BarTender, etc)
3. Print at 300 DPI for best quality
4. Apply to crosses

### DIY Printing

1. Print on regular paper (or photo paper)
2. Cut out and laminate with clear tape
3. Attach with weatherproof adhesive
4. Mount near the cross

## Testing QR Codes

### Scan and Verify

1. Use any QR code scanner app
2. Scan the printed QR
3. Verify it opens your PWA registration URL
4. Device name should pre-fill correctly

### Test on Phone

1. Open camera app (iOS) or use Google Lens (Android)
2. Point at QR code
3. Tap the notification that appears
4. Should navigate to PWA
5. Device should auto-populate

## Adding to Physical Cross

### Placement Ideas

1. **On the frame** - Direct attachment near top or base
2. **On an information card** - Mounted on stand
3. **On signage** - Large poster with QR and instructions
4. **Combination** - Multiple QR codes at different locations

### Weather Protection

- Laminate QR codes
- Use weatherproof stickers
- Cover with clear acrylic sheet
- Replace if faded

## Documentation Sticker

Consider creating a combo sticker:

```
         [QR CODE HERE]
    
    Register this CROSS
    Scan to add prayers
    
    Device: luke-3A7
```

## Batch Processing Example

Here's a complete workflow:

```bash
#!/bin/bash

# 1. Download device list from NocoDB
curl -H "xc-token: YOUR_TOKEN" \
  "https://noco.app.p7n.net/api/v2/tables/mdk4garchpkpvsd/records" \
  | jq -r '.list[] | "\(.device_name),\(.device_key)"' > devices.csv

# 2. Generate QR codes
while IFS=',' read -r name key; do
  echo "Generating QR for $name..."
  qrcode '{"device_key":"'"$key"'","register_url":"https://your-pwa.com/pwa-register"}' \
    -o "qr-$name.png"
done < devices.csv

# 3. Combine into single PDF (requires imagemagick)
convert qr-*.png qr-codes.pdf
```

## Files Generated

- `qr-luke-3a7.png` - Individual QR code (300×300px)
- `qr-codes.pdf` - Multi-page PDF ready to print

## Integration with Firmware

The firmware can serve QR codes via the `/api` endpoint:

```javascript
// In future firmware update:
server.on("/api/qr-code", HTTP_GET, handleQRCode);

void handleQRCode() {
  DynamicJsonDocument doc(256);
  doc["device_key"] = DEVICE_KEY;
  doc["device_name"] = DEVICE_NAME;
  doc["register_url"] = "https://your-pwa.com/pwa-register";
  
  // Generate SVG QR code...
  
  server.send(200, "image/svg+xml", qrSvg);
}
```

## Troubleshooting

**QR won't scan:**
- Try higher contrast (black/white)
- Increase size of QR code
- Ensure not damaged/faded
- Try different scanner app

**QR doesn't navigate to app:**
- Verify register_url is correct
- Check PWA is deployed and accessible
- Test URL in browser first

**Device name doesn't auto-fill:**
- Check device_name matches exactly in firmware
- Ensure device is registered in NocoDB
- Check `device_key` in QR matches firmware

## Next Steps

1. Deploy the PWA registration app
2. Generate QR codes for your crosses
3. Print and attach to physical devices
4. Test scanning with phone camera
5. Share PWA link with your community
