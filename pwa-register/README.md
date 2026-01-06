# CROSS PWA Registration App

A lightweight Progressive Web App (PWA) for users to register CROSS devices and submit prayers for intercession.

## Features

- **User Registration & Login** - Create accounts and authenticate
- **Cross Registration** - Link ESP32 CROSS devices to user accounts via device name or QR code scanning
- **Prayer Submission** - Submit prayer requests for specific crosses
- **My Crosses** - View all crosses registered to your account
- **QR Code Generation** - Generate QR codes for physical crosses to simplify registration
- **Offline Support** - Service worker enables basic offline functionality
- **Mobile-First Design** - Responsive design works on phones, tablets, and desktop

## Quick Start

### Prerequisites

1. **NocoDB Instance Running** - Must have NocoDB set up with the following tables:
   - `users` - User accounts
   - `crosses` - Device registration
   - `prayers` - Prayer submissions
   - `geo_areas` - Geographic areas (optional reference data)

2. **NocoDB Tables Setup**

Update the `CONFIG` in `app.js` with your actual NocoDB table IDs:

```javascript
const CONFIG = {
  NOCODB_URL: 'https://noco.app.p7n.net',
  API_TOKEN: 'your-token-here',
  
  USERS_TABLE: 'table-id-here',
  CROSSES_TABLE: 'table-id-here',
  PRAYERS_TABLE: 'table-id-here',
};
```

### Installation

#### Option 1: Local Development

```bash
cd pwa-register
python -m http.server 8000
# Visit http://localhost:8000
```

#### Option 2: Deploy to Static Host

Upload all files to any static hosting:
- Vercel (`vercel deploy`)
- Netlify (drag & drop)
- GitHub Pages
- Azure Static Web Apps
- Any traditional web host

#### Option 3: Serve from Tauri App

You can serve the PWA from your Tauri app's local server alongside the device provisioning interface.

## Database Schema

### Users Table

```
id          (auto)
name        (text)
email       (email, unique)
password    (text) - hash in production!
created_at  (timestamp)
```

### Crosses Table

```
id                  (auto)
name                (text) - display name
device_name         (text) - e.g., "luke-3A7"
device_key          (text) - unique hardware identifier
firmware_version    (text)
geo_area            (text) - geographic area assignment
location_notes      (text long)
user_id             (link to users)
status              (select: active/inactive/archived)
intercession_count  (number)
heartbeat_count     (number)
last_heartbeat      (timestamp)
created_at          (timestamp)
```

### Prayers Table

```
id              (auto)
prayer_text     (text long)
cross_id        (link to crosses)
user_id         (link to users)
status          (select: submitted/acknowledged/archived)
created_at      (timestamp)
```

## QR Code Format

When a QR code is generated for a cross, it encodes JSON data:

```json
{
  "device_key": "012345ABCDEF",
  "register_url": "https://your-pwa-domain.com/pwa-register"
}
```

The QR code can be:
- Printed and attached to the physical cross
- Scanned via mobile camera to start registration
- Used to quickly identify device_name during setup

## API Integration

The app communicates with NocoDB via the REST API:

```javascript
// Get records
GET /api/v2/tables/{tableId}/records?where=(field,eq,value)

// Create record
POST /api/v2/tables/{tableId}/records
Body: { field1: value1, field2: value2 }

// Update record
PATCH /api/v2/tables/{tableId}/records?where=(Id,eq,recordId)
Body: { field1: newValue }
```

All requests include: `xc-token: your-api-token`

## Demo Account

For testing:
- **Email:** test@cross.local
- **Password:** password123

## Customization

### Update Colors

Edit the CSS in `index.html`:

```css
background: #0f0f0f;  /* Dark background */
color: #eee;          /* Light text */
border: 1px solid #333;  /* Borders */
```

### Add Geographic Areas

Modify the select options in `index.html`:

```html
<select id="geoArea">
  <option value="north">North District</option>
  <option value="south">South District</option>
  <!-- Add your areas -->
</select>
```

### Connect to Real Backend

To integrate with a proper backend (recommended for production):

```javascript
// Replace NocoDB API calls with your backend
async function handleLogin() {
  const response = await fetch('https://your-api.com/auth/login', {
    method: 'POST',
    body: JSON.stringify({ email, password })
  });
  // ...
}
```

## Production Checklist

- [ ] Move database credentials to environment variables or backend
- [ ] Hash passwords using bcrypt
- [ ] Add proper authentication (JWT, sessions)
- [ ] Use HTTPS for all connections
- [ ] Add CORS headers if API is on different domain
- [ ] Implement rate limiting
- [ ] Add form validation and sanitization
- [ ] Test across devices and browsers
- [ ] Set up error logging/monitoring
- [ ] Add privacy policy and terms of service

## Security Notes

⚠️ **Current Implementation:**
- Uses plaintext password storage (development only!)
- API token exposed in client code
- No CSRF protection

✅ **Production Requirements:**
1. Create a backend API to:
   - Hash passwords with bcrypt
   - Manage JWT tokens
   - Validate all inputs
   - Control API token access
2. Use environment variables for sensitive data
3. Implement proper CORS
4. Add rate limiting
5. Use HTTPS everywhere

## Offline Behavior

The service worker caches:
- HTML, CSS, JS files
- Recent API responses
- QR code library

When offline:
- Users can view cached pages
- API calls will use cached data
- New data will sync when online

## Development Tips

### Debug

Open browser DevTools:
- **Network tab**: Monitor API calls to NocoDB
- **Application tab**: Inspect service worker & cache
- **Console**: See JS errors

### Test PWA Features

1. **Install app**: Click browser menu → "Install app"
2. **Go offline**: DevTools → Network → Offline
3. **Check cache**: DevTools → Application → Cache → cross-pwa-v1

## Troubleshooting

### "Cross device not found"
- Ensure cross is registered in NocoDB `crosses` table
- Check device_name matches exactly (case-sensitive)
- Verify device connected to WiFi and synced with NocoDB

### API requests failing
- Verify NocoDB instance is accessible
- Check API token is correct
- Ensure CORS is enabled if on different domain
- Check browser console for detailed errors

### Service worker not updating
```javascript
// Hard refresh
Ctrl+Shift+R (Windows/Linux)
Cmd+Shift+R (Mac)
```

## File Structure

```
pwa-register/
├── index.html       # Main UI
├── app.js           # Application logic
├── sw.js            # Service worker (offline support)
├── manifest.json    # PWA manifest
└── README.md        # This file
```

## Next Steps

1. **Update CONFIG in app.js** with your NocoDB details
2. **Test locally** with `python -m http.server 8000`
3. **Deploy** to your hosting provider
4. **Share URL** with users to register

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review browser console for errors
3. Verify NocoDB connection and API token
4. Check network requests in DevTools
