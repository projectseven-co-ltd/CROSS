### Adjusting for NocoDB Import Limitations

NocoDB does not allow converting a standard field (e.g., Number or Text) to a 'Link to another record' (relation) field after import. To set up links after importing CSVs:

1. **Delete the standard field:**
  - In the table, delete the field you want to convert (e.g., geo_area_id, user_id).
2. **Add a new link field:**
  - Add a new field with the same name, but set its type to 'Link to another record'.
  - Select the target table and field (usually id).
3. **Restore data:**
  - Manually re-link records using the UI, or re-import data mapping to the new link field.

This process is required for each field you want to convert to a relation after import.
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

## 2. Hardware & Power (ESP32 Cross)

### Core MCU
- ESP32 Nano (WiFi/BLE) + expansion header
- Supports I2C/SPI/UART for display/sensors
- Sleep-capable for low-power polling

### Power Architecture
- Dual 18650 Li-ion cells (parallel) for capacity ~5000-7000 mAh depending on cells
- Smart charger/BMS (TP4056-class with protection) per pack; include over-charge, over-discharge, short protection
- Recommended charger module: TP4056 + DW01A + 8205A “18650 Lithium Battery Charger Module” with separate BAT/OUT pads for load sharing (run device while charging)
- Power wiring: Connect BAT+ from TP4056 module directly to Arduino Nano ESP32 5V/VBUS pin (4.2V max, safe for onboard regulator). No external buck regulator needed; onboard LDO provides 3.3V for ESP32 and peripherals. Nano can run from battery or USB-C.
- If using VIN pin, ensure voltage is 6V or higher (not directly from single-cell Li-ion). For battery, use 5V/VBUS input.
- Battery gauge/voltage divider to report SOC to firmware and upstream API
- Power switch or load sharing so device can run while charging

### Low-Power Strategy
- Deep sleep between polls; target <200 µA sleep, <100 mA active bursts
- Duty-cycle radio: Status poll every 60-120s (tuneable) and heartbeat 5-10 min to extend runtime
- LED strategy: Prefer WS2812B short strip with low brightness, or single RGB LED; avoid full-brightness patterns
- Display: Favor e-ink or small OLED with brief updates; turn off backlight when idle
- Optional PIR to wake display/LEDs only when someone is nearby

### Interfaces on Expansion Board
- Battery input with JST or screw terminal, fuse inline
- Charger/BMS + test points
- 3.3V regulator output + 5V pass-through for LEDs if needed
- I2C header (SDA/SCL/3.3V/GND) for display or sensors
- GPIO breakout for LED strip data and optional buttons

### Power Budget (rough, per active burst)
- ESP32 active TX/RX WiFi: 120-240 mA
- ESP32 light sleep idle: ~2-10 mA (radio off)
- Deep sleep: ~150-250 µA with good regulator/BMS
- Single WS2812B LED @ low brightness: 5-20 mA; short strip (5 LEDs) up to 100 mA at modest brightness
- 1" OLED: 10-20 mA when lit

### Firmware Notes
- Use deep sleep; wake on timer for polling and heartbeat
- Batch requests: single HTTPS poll per wake, update LED/display, then sleep
- Report battery voltage every heartbeat to n8n/NocoDB for fleet health
- Configurable polling/heartbeat via remote config to balance power vs responsiveness

### Future Power Enhancements
- Solar trickle input for outdoor crosses
- Supercapacitor buffer for brief surges (LED/display)
- Optional LoRa add-on for long-range, low-power status when WiFi is weak

## 3. Database Schema (NocoDB)

**Note:** When importing CSVs, relation/link fields (e.g., geo_area_id, user_id) must be created as 'Link to another record' fields in NocoDB. If imported as standard fields, you must delete and recreate them as link fields to enable relationships. See 'Adjusting for NocoDB Import Limitations' below for details.

### Setting Up Table Relationships in NocoDB

#### 1. Link crosses.geo_area_id to geo_areas.id
  - Open the crosses table.
  - Edit the geo_area_id field.
  - Set the field type to "Relation" or "Link to another table".
  - Select geo_areas as the target table and id as the target field.
  - Save changes.

#### 2. Link crosses.user_id to users.id (optional/user-maintained)
  - Open the crosses table.
  - Edit the user_id field.
  - Set the field type to "Relation" or "Link to another table".
  - Select users as the target table and id as the target field.
  - Allow empty values for community crosses.
  - Save changes.

#### 3. Link prayers.user_id to users.id
  - Open the prayers table.
  - Edit the user_id field.
  - Set the field type to "Relation" or "Link to another table".
  - Select users as the target table and id as the target field.
  - Save changes.

#### 4. Link prayers.geo_area_id to geo_areas.id
  - Open the prayers table.
  - Edit the geo_area_id field.
  - Set the field type to "Relation" or "Link to another table".
  - Select geo_areas as the target table and id as the target field.
  - Save changes.

After these steps, your tables will be fully linked and you can use NocoDB’s relational features for lookups, filters, and automations.

### Table: crosses
**Relationships:**
- user_id: Many-to-One (many crosses can be maintained by one user; nullable for community crosses)
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| name | String | Cross name/location |
| user_id | Link to users (nullable) | Maintainer/owner if user-maintained, blank for community crosses |
| status | Single Select | active/inactive/error |
| last_heartbeat | DateTime | Last communication |
| firmware_version | String | ESP32 firmware version |

### Table: geo_areas
**Relationships:**
- id: One-to-Many (one geo_area has many crosses and many prayers)
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| name | String | Area name (city, region) |
| center_lat | Number | Geographic center |
| center_lon | Number | Geographic center |
| radius_km | Number | Search radius for crosses |
| prayer_threshold_low | Number | Threshold for color changes |
| prayer_threshold_high | Number | Threshold for color changes |

### Table: prayers
**Relationships:**
- user_id: Many-to-One (many prayers per user)
- geo_area_id: Many-to-One (many prayers per geo_area)
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| user_id | Link to users | Who submitted prayer |
| geo_area_id | Link to geo_areas | Target area |
| title | String | Prayer title |
| description | Text | Prayer request details |
| scripture_reference | String | Optional: Related Bible verse (Mark 8:34, Luke 9:23, etc.) |
| expires_at | DateTime | When prayer expires (auto-remove) |
| status | Single Select | active/completed/archived |
| burden_level | Single Select | light/moderate/heavy | (How urgent the prayer need is)
| intercession_count | Number | How many times this was interceded for |

### Table: users
**Relationships:**
- id: One-to-Many (one user can maintain many crosses and submit many prayers)
| Field | Type | Purpose |
|-------|------|---------|
| id | Number | Unique identifier |
| phone | String | Phone number (for app auth) |
| email | String | User email address |
| name | String | User name |

---

## 4. n8n Workflows

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

## 5. ESP32 Application Flow

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

## 6. Mobile App Endpoints

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

## 7. Configuration & Thresholds

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

## 8. Future Enhancements
- [ ] Daily devotional/scripture push notifications (Mark 8:34, Luke 9:23 themes)
- [ ] Prayer journal/testimony archive linked to crosses
- [ ] Community prayer vigil mode (extended watching/intercession periods)
- [ ] Integration with Bible API to display relevant verses
- [ ] Cross-community prayer chains and testimonies
- [ ] Admin dashboard with prayer analytics and community impact metrics
- [ ] Intercessory prayer training resources & guides
- [ ] Church integration: Display crosses in sanctuaries as visual prayer focal points

---

## 9. Development Phases

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

## 10. Open Questions to Finalize

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
