@echo off
REM CROSS PWA Setup Helper for Windows
REM Run this to get started quickly

cls
echo 🚀 CROSS PWA Registration Setup
echo ================================
echo.

REM Check if we're in the right directory
if not exist "app.js" (
    echo ❌ Error: Please run this from the pwa-register directory
    echo    cd pwa-register ^&^& setup.bat
    exit /b 1
)

echo ✅ Found PWA files
echo.

echo 📝 Please provide your NocoDB configuration:
echo.

set /p NOCODB_URL="NocoDB URL (e.g., https://noco.app.p7n.net): "
set /p API_TOKEN="API Token: "
set /p USERS_TABLE="Users Table ID: "
set /p CROSSES_TABLE="Crosses Table ID: "
set /p PRAYERS_TABLE="Prayers Table ID: "

echo.
echo 🔧 Updating configuration...

REM Create temporary file with new config
(
echo // CROSS Registration PWA App
echo // Simple vanilla JS app ^(no build tools needed^)
echo.
echo // Configuration
echo const CONFIG = {
echo   // Update these to match your NocoDB instance
echo   NOCODB_URL: '%NOCODB_URL%',
echo   API_TOKEN: '%API_TOKEN%',
echo.
echo   // Table IDs from NocoDB
echo   USERS_TABLE: '%USERS_TABLE%',
echo   CROSSES_TABLE: '%CROSSES_TABLE%',
echo   PRAYERS_TABLE: '%PRAYERS_TABLE%',
echo };
) > app_new.js

REM Append the rest of the original file (skip first 13 lines)
setlocal enabledelayedexpansion
set count=0
for /f "usebackq delims=" %%A in (app.js) do (
    set /a count+=1
    if !count! gtr 13 echo %%A>> app_new.js
)

REM Replace the file
del app.js
ren app_new.js app.js

echo ✅ Configuration updated
echo.
echo 📦 Testing local server...
echo.

REM Check for Python
python --version >nul 2>&1
if %errorlevel% equ 0 (
    echo 🌐 Starting development server on http://localhost:8000
    echo.
    echo 💡 Tips:
    echo    - Open http://localhost:8000 in your browser
    echo    - Use test@cross.local / password123 to login
    echo    - Press Ctrl+C to stop the server
    echo.
    python -m http.server 8000
) else (
    echo ⚠️ Python not found. Please start a local server manually:
    echo.
    echo   python -m http.server 8000
    echo   REM or if you have Node.js:
    echo   npx http-server
)
