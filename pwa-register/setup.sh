#!/bin/bash
# CROSS PWA Setup Helper
# Run this to get started quickly

set -e

echo "🚀 CROSS PWA Registration Setup"
echo "================================"
echo ""

# Check if we're in the right directory
if [ ! -f "app.js" ]; then
    echo "❌ Error: Please run this from the pwa-register directory"
    echo "   cd pwa-register && bash setup.sh"
    exit 1
fi

echo "✅ Found PWA files"
echo ""

# Ask for NocoDB details
echo "📝 Please provide your NocoDB configuration:"
echo ""

read -p "NocoDB URL (e.g., https://noco.app.p7n.net): " NOCODB_URL
read -p "API Token: " API_TOKEN
read -p "Users Table ID: " USERS_TABLE
read -p "Crosses Table ID: " CROSSES_TABLE
read -p "Prayers Table ID: " PRAYERS_TABLE

echo ""
echo "🔧 Updating configuration..."

# Update app.js with the provided values
cat > app.js.tmp << EOF
// CROSS Registration PWA App
// Simple vanilla JS app (no build tools needed)

// Configuration
const CONFIG = {
  // Update these to match your NocoDB instance
  NOCODB_URL: '$NOCODB_URL',
  API_TOKEN: '$API_TOKEN',
  
  // Table IDs from NocoDB
  USERS_TABLE: '$USERS_TABLE',
  CROSSES_TABLE: '$CROSSES_TABLE',
  PRAYERS_TABLE: '$PRAYERS_TABLE',
};
EOF

# Get the rest of app.js after the CONFIG section
tail -n +13 app.js >> app.js.tmp
mv app.js.tmp app.js

echo "✅ Configuration updated"
echo ""
echo "📦 Testing local server..."
echo ""

# Check if Python 3 is available
if command -v python3 &> /dev/null; then
    PORT=8000
    echo "🌐 Starting development server on http://localhost:$PORT"
    echo ""
    echo "💡 Tips:"
    echo "   - Open http://localhost:$PORT in your browser"
    echo "   - Use test@cross.local / password123 to login (if user exists)"
    echo "   - Press Ctrl+C to stop the server"
    echo ""
    
    python3 -m http.server $PORT
elif command -v python &> /dev/null; then
    PORT=8000
    echo "🌐 Starting development server on http://localhost:$PORT"
    echo ""
    python -m SimpleHTTPServer $PORT
else
    echo "⚠️  Python not found. Please start a local server manually:"
    echo ""
    echo "   python -m http.server 8000"
    echo "   # or"
    echo "   python3 -m http.server 8000"
    echo "   # or"
    echo "   npx http-server"
fi
