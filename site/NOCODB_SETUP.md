# NocoDB Table Setup for CROSS PWA

This guide helps you set up the required NocoDB tables for the PWA registration app.

## Required Tables

### 1. Users Table

**Create a table named `users` with these fields:**

| Field Name | Field Type | Settings |
|---|---|---|
| Id | Auto-increment | Primary key |
| name | Single line text | Required |
| email | Email | Required, Unique |
| password | Single line text | Required |
| created_at | Created time | Auto-filled |

```sql
-- If manually creating:
CREATE TABLE users (
  Id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL,
  email VARCHAR(255) NOT NULL UNIQUE,
  password VARCHAR(255) NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Demo user to create:**
- Name: Test User
- Email: test@cross.local
- Password: password123

### 2. Crosses Table

**Create a table named `crosses` with these fields:**

| Field Name | Field Type | Settings |
|---|---|---|
| Id | Auto-increment | Primary key |
| name | Single line text | - |
| device_name | Single line text | Should match ESP32 device_name |
| device_key | Single line text | Unique hardware identifier |
| firmware_version | Single line text | - |
| geo_area | Single line text | Geographic area assignment |
| location_notes | Long text | Optional location details |
| user_id | Lookup / Link | Links to users table |
| status | Single select | Options: active, inactive, archived |
| intercession_count | Number | Count of prayers |
| heartbeat_count | Number | Activity counter from device |
| last_heartbeat | DateTime | Last sync from device |
| created_at | Created time | Auto-filled |

```sql
CREATE TABLE crosses (
  Id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255),
  device_name VARCHAR(255),
  device_key VARCHAR(255) UNIQUE,
  firmware_version VARCHAR(50),
  geo_area VARCHAR(100),
  location_notes TEXT,
  user_id INT,
  status VARCHAR(50) DEFAULT 'active',
  intercession_count INT DEFAULT 0,
  heartbeat_count INT DEFAULT 0,
  last_heartbeat TIMESTAMP,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(Id)
);
```

### 3. Prayers Table

**Create a table named `prayers` with these fields:**

| Field Name | Field Type | Settings |
|---|---|---|
| Id | Auto-increment | Primary key |
| prayer_text | Long text | Prayer content |
| cross_id | Lookup / Link | Links to crosses table |
| user_id | Lookup / Link | Links to users table |
| status | Single select | Options: submitted, acknowledged, archived |
| created_at | Created time | Auto-filled |

```sql
CREATE TABLE prayers (
  Id INT PRIMARY KEY AUTO_INCREMENT,
  prayer_text TEXT NOT NULL,
  cross_id INT,
  user_id INT,
  status VARCHAR(50) DEFAULT 'submitted',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (cross_id) REFERENCES crosses(Id),
  FOREIGN KEY (user_id) REFERENCES users(Id)
);
```

### 4. (Optional) Geo Areas Reference Table

**For managing geographic areas:**

| Field Name | Field Type | Settings |
|---|---|---|
| Id | Auto-increment | Primary key |
| name | Single line text | Area name |
| description | Long text | Area description |
| region | Single line text | Region/district |

```sql
CREATE TABLE geo_areas (
  Id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255),
  description TEXT,
  region VARCHAR(100)
);
```

## NocoDB Setup Steps

### Step 1: Create Tables via NocoDB UI

1. Open your NocoDB instance
2. Click "+" to create a new table
3. Name it "users"
4. Add fields one by one using the "+" button
5. Repeat for "crosses" and "prayers" tables

### Step 2: Link Tables

1. In the "crosses" table:
   - Click "+" to add a new column
   - Set type to "Lookup"
   - Select "users" table and "Id" field
   - This creates a link from crosses → users

2. In the "prayers" table:
   - Add two Lookup columns:
     - One linking to "crosses"
     - One linking to "users"

### Step 3: Get Table IDs

For the PWA `app.js` config:

1. Open NocoDB → Your base
2. Click on the "users" table
3. Look at the URL: `?base=BASE_ID&nc=<TABLE_ID>`
4. Copy the TABLE_ID value
5. Repeat for "crosses" and "prayers"

**Example URL:**
```
https://noco.app.p7n.net?base=mjuirvwtkgxs1d&nc=mtsl8eo5bfxhg8h
                                              ↑
                                    Table ID to copy
```

### Step 4: Update app.js

In `pwa-register/app.js`, update:

```javascript
const CONFIG = {
  NOCODB_URL: 'https://noco.app.p7n.net',
  API_TOKEN: 'your-token-here',
  
  USERS_TABLE: 'mtsl8eo5bfxhg8h',      // ← Paste users table ID
  CROSSES_TABLE: 'mdk4garchpkpvsd',    // ← Paste crosses table ID
  PRAYERS_TABLE: 'mt7iucs253e26u0',    // ← Paste prayers table ID
};
```

### Step 5: Create API Token

1. In NocoDB, go to Settings → Tokens
2. Create a new token
3. Copy the token
4. Update `API_TOKEN` in `app.js`

## Integration with ESP32 Firmware

The firmware automatically:

1. Creates a "crosses" record when it boots
2. Uses `device_key` (from MAC address) as unique identifier
3. Updates `firmware_version` when it syncs
4. Increments `intercession_count` when prayers arrive
5. Updates `last_heartbeat` every 60 seconds

The PWA uses these fields to:
- Find crosses by `device_name`
- Link prayers to crosses
- Display prayer counts

## Field Descriptions

### Users Table
- **email**: Unique login identifier
- **password**: Should be hashed in production!

### Crosses Table
- **device_name**: Matches ESP32 `DEVICE_NAME` (e.g., "luke-3A7")
- **device_key**: Hardware MAC address (12 hex characters)
- **geo_area**: User's assigned region for the cross
- **user_id**: Which user registered this cross
- **status**: "active" = receiving prayers, "inactive" = paused
- **intercession_count**: Total prayers received by this cross

### Prayers Table
- **prayer_text**: The actual prayer request
- **status**: "submitted" = new, "acknowledged" = seen by user
- **cross_id**: Which cross this prayer is for

## Example Data

### Sample Users
```
id | name      | email              | password
1  | John Doe  | john@cross.local   | hash123
2  | Jane Smith| jane@cross.local   | hash456
```

### Sample Crosses
```
id | device_name | device_key      | user_id | geo_area | status
1  | luke-3A7    | A1B2C3D4E5F6    | 1       | north    | active
2  | john-F2C5   | F2C5D6E7F8G9    | 2       | south    | active
```

### Sample Prayers
```
id | prayer_text           | cross_id | user_id | status
1  | Please pray for...    | 1        | 2       | submitted
2  | Intercede for...      | 1        | 1       | submitted
```

## Troubleshooting

**Issue: "Table not found"**
- Verify table ID is correct
- Check API token has access
- Ensure table exists in NocoDB

**Issue: "Field not found"**
- Check field names match exactly (case-sensitive)
- Ensure all required fields are created

**Issue: "API Token invalid"**
- Regenerate token in NocoDB Settings
- Ensure token is not expired
- Check it's copied exactly (no spaces)

## Next Steps

1. Create all three tables with fields above
2. Get the table IDs and API token
3. Update `app.js` with your IDs
4. Create test user account
5. Deploy the PWA
6. Test registration flow
