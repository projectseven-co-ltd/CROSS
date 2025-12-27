# CROSS - Prayer Network Project Plan

## Biblical Foundation

> **Mark 8:34** - "Then he called the crowd to him along with his disciples and said: 'Whoever wants to be my disciple must deny themselves and take up their cross and follow me.'"

> **Luke 9:23** - "Then he said to them all: 'Whoever wants to be my disciple must deny themselves and take up their cross daily and follow me.'"

**Mission**: CROSS is a community prayer network that embodies the call to take up one's cross daily. Each physical cross represents a commitment to intercessory prayer, community support, and bearing one another's burdens through shared spiritual witness.

---

## Project Overview
ESP32-based prayer crosses that form a geo-aware network. Users submit prayer requests via mobile app, and crosses respond with visual/status indicators (light, color, display) based on prayer activity in their geographic area. This creates a visible, tangible representation of the body of Christ bearing one another's crosses through prayer.

---

## 1. System Architecture

### Components
- **ESP32 Devices** (Prayer Crosses)
- **n8n Workflow Engine** (Business logic & API layer)
- **NocoDB Database** (Data persistence)
- **Mobile App** (User interface for prayer submission)

### Data Flow
```
Mobile App → n8n API → NocoDB (store prayer)
    ↓
n8n Webhook/Logic → Update cross area status
    ↓
ESP32 polls n8n → Gets area prayer count/status
    ↓
ESP32 Updates (LED color, display, etc.)
```

---

## 2. Database Schema (NocoDB)

### Table: crosses
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| name | String | Cross name/location |
| latitude | Number | GPS coordinate |
| longitude | Number | GPS coordinate |
| geo_area_id | Link to geo_areas | Which area this cross belongs to |
| status | Single Select | active/inactive/error |
| last_heartbeat | DateTime | Last communication |
| color_state | String | Current LED color |
| firmware_version | String | ESP32 firmware version |

### Table: geo_areas
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| name | String | Area name (city, region) |
| center_lat | Number | Geographic center |
| center_lon | Number | Geographic center |
| radius_km | Number | Search radius for crosses |
| current_prayer_count | Number | Active prayers in area |
| prayer_threshold_low | Number | Threshold for color changes |
| prayer_threshold_high | Number | Threshold for color changes |

### Table: prayers
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| user_id | Link to users | Who submitted prayer |
| geo_area_id | Link to geo_areas | Target area |
| title | String | Prayer title |
| description | Text | Prayer request details |
| scripture_reference | String | Optional: Related Bible verse (Mark 8:34, Luke 9:23, etc.) |
| created_at | DateTime | When submitted |
| expires_at | DateTime | When prayer expires (auto-remove) |
| status | Single Select | active/completed/archived |
| prayer_count | Number | How many times this was interceded for |
| burden_level | Single Select | light/moderate/heavy | (How urgent the prayer need is)

### Table: users
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| phone | String | Phone number (for app auth) |
| name | String | User name |
| created_at | DateTime | Registration date |
| prayer_count | Number | Total prayers submitted |

---

## 3. n8n Workflows

### Workflow 1: Prayer Submission
**Trigger**: Mobile app POSTs prayer to n8n endpoint
**Logic**:
1. Validate prayer data
2. Create prayer record in NocoDB
3. Identify geo_area from coordinates
4. Increment prayer_count in geo_area
5. Return success + area_id

**Output**: `{ prayer_id, geo_area_id, current_prayer_count }`

### Workflow 2: Area Status Check (ESP32 Polling)
**Trigger**: ESP32 POSTs to n8n endpoint with its geo_area_id
**Logic**:
1. Query NocoDB for all active prayers in that geo_area
2. Get all crosses in that geo_area
3. Calculate prayer_count
4. Determine color/status based on thresholds
5. Return status to ESP32

**Output**: `{ prayer_count, suggested_color, crosses_in_area: [] }`

### Workflow 3: Cross Heartbeat
**Trigger**: ESP32 POSTs heartbeat every X minutes
**Logic**:
1. Update cross.last_heartbeat timestamp
2. Update cross.status = "active"
3. Get area status (run Workflow 2 logic)

**Output**: `{ area_status, pending_updates }`

### Workflow 4: Prayer Expiration (Scheduled)
**Trigger**: Runs every 5 minutes
**Logic**:
1. Find all prayers where expires_at < now
2. Mark as "archived"
3. Recalculate prayer_count for affected geo_areas

---

## 4. ESP32 Application Flow

### Setup Phase
1. WiFi connection
2. Get GPS coordinates (if available)
3. Calculate geo_area or hardcode geo_area_id
4. Register cross with n8n (get cross_id back)

### Main Loop
1. Every 30-60 seconds: Poll area status from n8n
2. Parse response (prayer_count, suggested_color)
3. Update LED/display based on thresholds
4. Every 5 minutes: Send heartbeat

### Color Mapping (Configurable)
- **No prayers**: Blue (Waiting/Ready to bear)
- **1-5 prayers**: Green (Growing burden/Active intercession)
- **6-10 prayers**: Yellow (Heavier burden/Many in need)
- **11+ prayers**: Red (Heavy cross/Urgent need for prayer warriors)

*Each color represents the community's call to "take up the cross" and bear burdens together.*

---

## 5. Mobile App Endpoints

### POST /prayers
Submit a prayer request to take up in your community
```json
{
  "user_id": "123",
  "latitude": 40.7128,
  "longitude": -74.0060,
  "title": "Strength in Trials",
  "description": "Pray for Sarah as she faces a difficult season. May she find strength in Christ.",
  "scripture_reference": "Mark 8:34",
  "burden_level": "heavy"
}
```

### GET /area/:geo_area_id
Get area's prayer needs and cross status
```json
{
  "geo_area_id": "456",
  "prayer_count": 7,
  "crosses_count": 3,
  "current_burden": "heavy",
  "crosses": [
    { "id": "1", "name": "Downtown Chapel", "color": "red", "active_prayers": 3 }
  ],
  "message": "Take up your cross daily and follow Him - Luke 9:23"
}
```

### GET /user/:user_id/prayers
Get user's prayer history and intercessions

---

## 6. Configuration & Thresholds

### Prayer Expiration
- Default: 24 hours
- Configurable per prayer or globally

### ESP32 Polling Interval
- Status check: 30-60 seconds
- Heartbeat: 5 minutes
- Adjustable based on power/connectivity

### Color Thresholds
- Configurable per geo_area in NocoDB
- Can be different for different areas

---

## 7. Future Enhancements
- [ ] Daily devotional/scripture push notifications (Mark 8:34, Luke 9:23 themes)
- [ ] Prayer journal/testimony archive linked to crosses
- [ ] Community prayer vigil mode (extended watching/intercession periods)
- [ ] Integration with Bible API to display relevant verses
- [ ] Cross-community prayer chains and testimonies
- [ ] Admin dashboard with prayer analytics and community impact metrics
- [ ] Intercessory prayer training resources & guides
- [ ] Church integration: Display crosses in sanctuaries as visual prayer focal points

---

## 8. Development Phases

### Phase 1: MVP
- NocoDB schema setup
- n8n: Prayer submission workflow
- n8n: Area status workflow
- Basic ESP32 polling logic
- Simple mobile app (web form)

### Phase 2: Polish
- ESP32 LED color implementation
- Heartbeat & health monitoring
- User authentication
- Prayer expiration logic

### Phase 3: Scale
- Real-time updates
- Analytics
- Admin dashboard
- Cross-area queries

---

## 9. Open Questions to Finalize

### Geo-Area Calculation
- [ ] Should ESP32s determine area by GPS proximity calculation?
- [ ] Or hardcode geo_area_id?
- [ ] How accurate does GPS need to be?

### Color Thresholds
- [ ] Same globally or different per area?
- [ ] Should thresholds be time-based (peak hours)?

### Prayer Lifespan
- [ ] Default expiration time? (24 hours?)
- [ ] Can users manually complete prayers?
- [ ] Should old prayers archive or delete?

### ESP32 Hardware
- [ ] RGB LED strip?
- [ ] LCD/OLED display?
- [ ] Speaker for audio notifications?
- [ ] Multiple LEDs for different states?

### Power Considerations
- [ ] Battery or mains powered?
- [ ] Sleep modes to conserve power?
- [ ] WiFi always on or periodic wake?

---

## 10. Theological Integration

### Core Biblical Principle
The CROSS project is grounded in the scriptural call to intercession and community burden-bearing:

- **Mark 8:34 & Luke 9:23**: "Deny yourself, take up your cross, and follow me"
- **Galatians 6:2**: "Carry each other's burdens, and in this way you will fulfill the law of Christ"
- **1 Thessalonians 5:17**: "Pray without ceasing"
- **James 5:16**: "Therefore confess your sins to each other and pray for each other that you may be healed"

### Application to CROSS
- **Visual Prayer**: Illuminated crosses in the community serve as physical reminders of Christ's call and our obligation to intercede
- **Burden Sharing**: The prayer count and color intensity represent the literal "weight" the community is carrying together
- **Daily Commitment**: Each cross that lights up reinforces the daily call to "take up your cross"
- **Community of Prayer**: Geo-aware crosses create prayer networks that strengthen local church communities

### Prayer Categories (Optional)
Users can categorize prayers by type:
- Salvation & Discipleship
- Persecution & Suffering
- Healing & Wholeness
- Leadership & Wisdom
- Community & Unity
- National/Global Concerns
