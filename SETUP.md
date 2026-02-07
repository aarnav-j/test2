# 📋 Setup Guide - IoT Data Relay System

Complete step-by-step instructions to get the relay system running.

## 🎯 Prerequisites

- **Node.js** (v14+) - Download from nodejs.org
- **Git** - For version control
- **Arduino IDE** - For ESP32 programming
- **ESP32 Board** - DOIT ESP32 DEVKIT V1 or similar
- **OLED Display** - 128x64 I2C display (optional, for receiving ESP32)

---

## 🖥️ Backend Setup (Local)

### Step 1: Extract Project
Make sure you have the complete `task2` folder with:
```
task2/
├── backend/
├── dashboard/
├── esp32/
└── README.md
```

### Step 2: Navigate to Backend Folder
```bash
cd task2/backend
```

### Step 3: Install Dependencies
```bash
npm install
```

This installs:
- express
- cors
- axios

### Step 4: Configure Environment
Create `.env` file (copy from `.env.example`):
```bash
# Windows
copy .env.example .env

# Mac/Linux
cp .env.example .env
```

Edit `.env`:
```
API_KEY=your-secret-api-key-12345
PORT=3001
```

### Step 5: Verify Primary Backend URL
Open `server.js` and check line 11:
```javascript
const SOURCE_BACKEND_URL = 'https://test-fpbw.onrender.com/api/latest-data';
```

Make sure this is correct (it should be your deployed primary backend URL from Task 1)

### Step 6: Start Backend
```bash
npm start
```

You should see:
```
✓ IoT Data Relay Backend running on http://localhost:3001

Configuration:
  Source Backend: https://test-fpbw.onrender.com/api/latest-data
  Poll Interval: 2000ms
  API Key: ✓ Set

Endpoints:
  GET  /api/latest-data
  POST /api/register-esp
  POST /api/send-data
  GET  /api/health
```

✅ **Backend is running!**

---

## 💻 Dashboard Setup (Local)

### Step 1: Open Browser
Go to: `http://localhost:3001`

### Step 2: Configure Backend URL (if needed)
If backend is on different machine, edit `dashboard/index.html`:

Find line 263:
```javascript
const BACKEND_URL = 'http://localhost:3001';
```

Change to your backend URL.

### Step 3: Verify Connection
Dashboard should show:
- Status: "Connecting..." or "Connected"
- Loading spinner or sensor data

If you see "Waiting for ESP32 data..." - that's normal! Data will appear once primary backend sends it.

✅ **Dashboard is ready!**

---

## 📱 ESP32 (Receiving) Setup

### Step 1: Install Arduino IDE
Download from https://www.arduino.cc/en/software

### Step 2: Install ESP32 Board
1. Open Arduino IDE
2. Go to **File** → **Preferences**
3. Add this URL to "Additional Boards Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Click **OK**
5. Go to **Tools** → **Board Manager**
6. Search for "ESP32"
7. Click **Install** (by Espressif Systems)

### Step 3: Install Required Libraries
1. Go to **Sketch** → **Include Library** → **Manage Libraries**
2. Search and install these libraries:
   - **ArduinoJson** (by Benoit Blanchon)
   - **Adafruit SSD1306** (by Adafruit)
   - **Adafruit GFX Library** (by Adafruit)
   - **Wire** (usually pre-installed)

### Step 4: Connect OLED to ESP32

**Wiring Diagram:**
```
OLED Display          ESP32
─────────────         ─────
   SDA ──────→ GPIO 21
   SCL ──────→ GPIO 22
   GND ──────→ GND
   VCC ──────→ 3.3V
```

### Step 5: Update Sketch with WiFi Credentials

Open `esp32/esp32_data_receiver.ino` in Arduino IDE.

Update **Line 27-28** with your WiFi:
```cpp
const char* WIFI_SSID = "HomeNetwork";        // ← YOUR WIFI NAME
const char* WIFI_PASSWORD = "Password123";     // ← YOUR WIFI PASSWORD
```

Update **Line 33-34** with relay backend URL:
```cpp
const char* BACKEND_URL = "https://your-relay-backend.onrender.com";  // ← YOUR RELAY BACKEND
const char* API_KEY = "your-secret-api-key-12345";  // ← SAME AS BACKEND
```

### Step 6: Select Board
1. Go to **Tools** → **Board**
2. Select: **ESP32** → **DOIT ESP32 DEVKIT V1** (or your model)
3. Go to **Tools** → **Port**
4. Select the COM port where ESP32 is connected

### Step 7: Upload Sketch
1. Click **Upload** button (arrow icon)
2. Wait for compilation and upload to complete
3. You should see: "✓ Hard resetting via RTS pin..."

### Step 8: Monitor Serial Output
1. Click **Tools** → **Serial Monitor**
2. Set baud rate to **115200** (bottom right)
3. You should see:
   ```
   ========================================
   IoT Data Receiver - ESP32
   ========================================
   Connecting to WiFi: HomeNetwork
   .................
   ✓ WiFi connected!
   IP address: 192.168.1.100
   Server listening on port 8080
   ✓ Ready to receive data on port 8080
   ```

✅ **Receiving ESP32 is ready!**

---

## 🚀 End-to-End Testing (Local)

### What Should Happen:
1. Primary ESP32 sends data to `https://test-fpbw.onrender.com/api/sensor-data`
2. Relay backend polls primary backend every 2 seconds
3. Relay backend receives data and updates dashboard
4. Relay backend broadcasts data to receiving ESP32 on port 8080
5. Receiving ESP32 displays data on OLED

### Test Data Flow:

**Terminal 1 (Backend):**
```
[2026-02-07T10:30:45.123Z] Data fetched from source: { temperature: 25.3, ... }
✓ Data sent to: http://192.168.1.100:8080
```

**Terminal/Serial Monitor (ESP32):**
```
--- Received Data ---
POST /api/receive HTTP/1.1

✓ Data parsed successfully:
  Temperature: 25.3
  Heart Rate: 72
  Motion: true
  Button: false
```

**Browser (Dashboard):**
- Shows live temperature, heart rate, motion, button
- Updates every 2 seconds

---

## 🌐 Deploying to Render

### Step 1: Push to GitHub
```bash
cd task2
git init
git add .
git commit -m "Initial commit - IoT Relay"
git push -u origin main
```

### Step 2: Create Render Account
Go to https://render.com and sign up

### Step 3: Create Web Service
1. Click **New +** → **Web Service**
2. Connect your GitHub repository
3. Fill in details:

**Name:** `iot-relay-backend` (or any name)

**Build Command:** `npm install && cd backend`

**Start Command:** `npm start`

**Environment Variable:**
```
Name: API_KEY
Value: your-secret-api-key-12345
```

4. Select **Free** plan
5. Click **Create Web Service**

### Step 4: Wait for Deployment
Render will deploy your application. Wait 2-5 minutes.

You'll get a URL like: `https://iot-relay-backend.onrender.com`

### Step 5: Update ESP32 Code
In `esp32_data_receiver.ino`, update line 33:
```cpp
const char* BACKEND_URL = "https://iot-relay-backend.onrender.com";
```

Re-upload to ESP32.

### Step 6: Test Public URLs
Open in browser:
```
https://iot-relay-backend.onrender.com/
https://iot-relay-backend.onrender.com/api/health
https://iot-relay-backend.onrender.com/api/latest-data
```

✅ **Your relay system is live!**

---

## 📋 Quick Checklist

### For Task Completion:

- [ ] Backend installs without errors: `npm install`
- [ ] Backend starts: `npm start`
- [ ] Dashboard accessible: `http://localhost:3001`
- [ ] ESP32 code uploads successfully
- [ ] ESP32 connects to WiFi (check serial monitor)
- [ ] ESP32 listening on port 8080
- [ ] Data flows from primary → relay → ESP32 → OLED
- [ ] Deployed to Render successfully
- [ ] Public URLs working

---

## 🆘 Common Issues & Fixes

### Backend won't start
```bash
# Check Node.js version
node --version  # Should be v14+

# Clear npm cache
npm cache clean --force

# Reinstall dependencies
rm -rf node_modules
npm install

# Try starting again
npm start
```

### Dashboard shows "Waiting for data..."
- Check primary backend URL in `server.js` is correct
- Check primary backend is running
- Check network connectivity
- Wait 2-3 seconds for first data poll

### ESP32 connection fails
- Check WiFi SSID and password are correct
- Check ESP32 is powered on
- Check WiFi is 2.4GHz (5GHz not supported)
- Reset ESP32: Press RESET button

### OLED display not working
- Check SDA/SCL wiring (GPIO 21/22)
- Check I2C address (default 0x3C)
- Install Adafruit libraries
- Check display power (3.3V)

### Port 3001 already in use
```bash
# Windows: Find and kill process on port 3001
netstat -ano | findstr :3001
taskkill /PID <PID> /F

# Mac/Linux
lsof -i :3001
kill -9 <PID>
```

---

## 📞 Next Steps

1. **Test locally** with all components
2. **Deploy relay backend** to Render
3. **Update ESP32** with your Render URL
4. **Monitor data flow** through dashboard
5. **Customize** dashboard or ESP32 code as needed

For troubleshooting, check the main [README.md](README.md).
