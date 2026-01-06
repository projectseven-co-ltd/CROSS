# CROSS PWA Registration - Quick Reference

## 📱 For Users

### Get Started
1. Visit: `https://your-pwa-url.com`
2. Sign up or log in
3. Register your CROSS (scan QR or enter device name)
4. Submit prayers

### Install as App
- **iPhone**: Share → Add to Home Screen
- **Android**: Menu (⋮) → Install App
- **Desktop**: Click install in address bar

## 🔧 For Developers

### File Structure
```
pwa-register/
├── index.html          Main UI
├── app.js              Logic & API calls
├── sw.js               Service worker (offline)
├── manifest.json       PWA metadata
├── README.md           Full documentation
├── DEPLOYMENT.html     Deploy guide
├── NOCODB_SETUP.md     Database setup
└── QR_GENERATION.md    QR code guide
```

### Quick Setup
```bash
# 1. Edit app.js with your NocoDB details
# Update CONFIG object with your:
# - NOCODB_URL
# - API_TOKEN
# - Table IDs

# 2. Test locally
python -m http.server 8000
# Visit http://localhost:8000

# 3. Deploy
# Use Vercel, Netlify, GitHub Pages, etc.
```

### Configuration
```javascript
const CONFIG = {
  NOCODB_URL: 'https://your-noco.com',
  API_TOKEN: 'your-token',
  USERS_TABLE: 'table-id-1',
  CROSSES_TABLE: 'table-id-2', 
  PRAYERS_TABLE: 'table-id-3',
};
```

## 🗄️ Database Schema

### Users
- id, name, email, password, created_at

### Crosses  
- id, device_name, device_key, user_id
- geo_area, location_notes, status
- intercession_count, last_heartbeat

### Prayers
- id, prayer_text, cross_id, user_id
- status, created_at

## 🚀 Deployment Options

| Service | Time | Cost | Notes |
|---------|------|------|-------|
| Vercel | 1 min | Free | Recommended |
| Netlify | 2 min | Free | Drag & drop |
| GitHub Pages | 5 min | Free | Good for repos |
| Azure Static | 5 min | Free | Azure integration |
| Traditional | 10 min | $$ | FTP upload |

## 📋 Checklist

- [ ] Create NocoDB tables
- [ ] Get API token and table IDs
- [ ] Update CONFIG in app.js
- [ ] Test locally
- [ ] Deploy to hosting
- [ ] Generate QR codes for crosses
- [ ] Print and attach QR codes
- [ ] Share PWA URL with community
- [ ] Test on mobile devices
- [ ] Add to home screen on phone

## 🔐 Security Notes

⚠️ **Current:**
- Plaintext passwords (dev only)
- API token in client code

✅ **Production needs:**
- Backend API with bcrypt password hashing
- JWT tokens for auth
- Environment variables for secrets
- CORS configuration
- Rate limiting
- Input validation

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| "Cross not found" | Check device_name matches firmware |
| "API error" | Verify NocoDB URL and token |
| "Won't install" | Try different browser |
| "Offline broken" | Clear cache, update SW |

## 📞 Support Files

1. **README.md** - Full documentation
2. **DEPLOYMENT.html** - Step-by-step deploy guide
3. **NOCODB_SETUP.md** - Database configuration
4. **QR_GENERATION.md** - QR code instructions

## 💡 Tips

- Service worker caches files for offline use
- Works with your existing CROSS firmware
- Integrates with NocoDB directly
- No backend needed (NocoDB is your backend)
- PWA works like native app on mobile

## 🔗 Links

- Local: `http://localhost:8000`
- After deploy: `https://your-domain.com/pwa-register`
- PWA installation: Available after deploy to HTTPS

---

**Questions?** Check the detailed README.md or DEPLOYMENT.html
