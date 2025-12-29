# CROSS Project API Endpoints

This document defines the RESTful API endpoints for the CROSS project, covering users, crosses, geo_areas, and prayers. All endpoints assume JSON request/response bodies unless otherwise noted.

---

## Users

- `GET /users` — List all users
- `GET /users/{user_id}` — Get a single user
- `POST /users` — Create a new user
- `PUT /users/{user_id}` — Update a user
- `DELETE /users/{user_id}` — Delete a user

## Crosses

- `GET /crosses` — List all crosses
- `GET /crosses?cross_id={cross_id}` — Get a single cross
- `POST /crosses` — Create a new cross
- `PUT /crosses?cross_id={cross_id}` — Update a cross
- `DELETE /crosses?cross_id={cross_id}` — Delete a cross
- `GET /crosses?crossid={cross_id}` — Cross device sends heartbeat (updates last_heartbeat timestamp)

### Heartbeat Endpoint Details
**Endpoint:** `GET /crosses?crossid={cross_id}`  
**Method:** GET  
**Query Params:** `crossid` (cross identifier)  
**Response:**
```json
{
  "last_heartbeat": {
    "value": "2025-12-28T20:15:34.042-07:00"
  }
}
```
**Example:** `GET https://n8n.app.p7n.net/webhook/c570d860-81e0-4e95-b04d-13014f3b373c/crosses?crossid=2`

## Geo Areas

- `GET /geo_areas` — List all geo areas
- `GET /geo_areas/{geo_id}` — Get a single geo area
- `POST /geo_areas` — Create a new geo area
- `PUT /geo_areas/{geo_id}` — Update a geo area
- `DELETE /geo_areas/{geo_id}` — Delete a geo area
- `GET /geo_areas/{geo_id}/status` — Get area status (prayer count, crosses, etc.)

## GET /geo_areas?geo_id={geo_id}/status` — ESP32 polls for area status (returns prayer count, suggested color, etc.)
- `POST /prayers?prayer_id={prayer_id}&action=
- `GET /prayers` — List all prayers
- `GET /prayers/{prayer_id}` — Get a single prayer
- `POST /prayers` — Create a new prayer
- `PUT /prayers/{prayer_id}` — Update a prayer
- `DELETE /prayers/{prayer_id}` — Delete a prayer
- `GET /users/{user_id}/prayers` — List all prayers submitted by a user
- `GET /geo_areas/{geo_id}/prayers` — List all prayers for a geo area

---

## Custom/Workflow Endpoints

- `POST /crosses/{cross_id}/status` — ESP32 polls for area status (returns prayer count, suggested color, etc.)
- `POST /prayers/{prayer_id}/intercede` — Increment intercession count for a prayer

---

This structure supports all core app and device flows. Expand as needed for authentication, admin, or analytics features.
