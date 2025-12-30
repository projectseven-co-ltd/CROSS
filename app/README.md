# Cross Finder - Tauri App

A lightweight desktop and mobile app for finding crosses in your vicinity using GPS coordinates.

## Features

- Minimalist black and white design
- Search for crosses by latitude and longitude
- Configurable search radius (1-100 km)
- Distance calculation using haversine formula
- Results sorted by proximity
- Cross-platform (desktop and mobile)

## Project Structure

```
app/
├── src/                    # Frontend (Vue/vanilla JS)
│   ├── main.js            # Main application logic
│   └── style.css          # Black and white styling
├── src-tauri/             # Tauri backend (Rust)
│   ├── src/
│   │   └── main.rs        # Backend with search logic
│   └── Cargo.toml         # Rust dependencies
├── index.html             # Entry HTML
├── package.json           # Node dependencies
└── vite.config.js         # Build configuration
```

## Setup Instructions

### Prerequisites

- Node.js 16+
- Rust (for Tauri backend)
- Your logo file placed at `/Volumes/Olson/GITSTUFF/CROSS/app/logo.svg`

### Installation

1. Navigate to the app directory:
```bash
cd /Volumes/Olson/GITSTUFF/CROSS/app
```

2. Install dependencies:
```bash
npm install
```

3. Install Tauri CLI globally (optional):
```bash
npm install -g @tauri-apps/cli
```

### Development

Run the development server:
```bash
npm run dev
```

This will start both the Vite dev server and Tauri dev window.

### Build

Create production builds:
```bash
npm run build
```

This generates:
- Desktop app for your OS in `src-tauri/target/release/`
- Web build in `dist/`

## Data Format

The app expects a `crosses.csv` file in the `../schema/` directory with columns:
```
id,name,latitude,longitude,description
```

Example:
```
1,Main Cross,40.7128,-74.0060,Times Square Area
2,Historic Cross,40.7580,-73.9855,Central Park
```

## Usage

1. Enter your current latitude and longitude
2. Set the search radius in kilometers
3. Click "Search"
4. View results sorted by distance

## Customization

### Color Scheme

Edit `src/style.css` to modify colors:
```css
--color-bg: #ffffff;      /* Background */
--color-text: #000000;    /* Text */
--color-primary: #000000; /* Button/accent */
```

### Logo

Place your logo at `/Volumes/Olson/GITSTUFF/CROSS/app/logo.svg` and it will display in the header.

## Notes

- No emojis used in the UI
- Simplistic, clean design focused on functionality
- Supports both desktop and mobile builds through Tauri
- Backend handles heavy lifting (distance calculations, CSV parsing)
