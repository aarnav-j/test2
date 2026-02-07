# 🔄 IoT Data Relay System (Task 2)

A **two-way IoT data relay system** that:
1. **Receives** sensor data from a primary ESP32 (via your deployed backend)
2. **Displays** data on a minimal dashboard
3. **Sends** data to a receiving ESP32 in real-time

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────┐
│ ESP32 User 1 (Sensor Reader)                        │
│ Sends: Temperature, Heart Rate, Motion, Button     │
└────────────┬────────────────────────────────────────┘
             │ HTTPS POST
             ↓
┌─────────────────────────────────────────────────────┐
│ Primary Backend (test-fpbw.onrender.com)            │
│ Stores latest sensor data                          │
└────────────┬────────────────────────────────────────┘
             │ GET /api/latest-data (polls every 2s)
             ↓
┌─────────────────────────────────────────────────────┐
│ Relay Backend (This Project - Task 2)               │
│ • Fetches data from primary backend                │
│ • Displays on dashboard                            │
│ • Broadcasts to receiving ESP32 devices            │
└────────────┬────────────────────────────────────────┘
             │ HTTP POST (data broadcast)
             ↓
┌─────────────────────────────────────────────────────┐
│ ESP32 User 2 (Receiving + OLED Display)            │
│ Receives: Temperature, Heart Rate, Motion, Button │
│ Displays on OLED 128x64 screen                     │
└─────────────────────────────────────────────────────┘
```

## ✨ Features

- ✅ **Real-time Data Flow** - Data flows from primary backend to receiving ESP32 in <2 seconds
- ✅ **API Key Protection** - Only authorized ESP32s can register and receive data
- ✅ **Minimal Dashboard** - Clean, responsive web interface
- ✅ **Auto-Broadcasting** - Data automatically sent to all registered ESP32 receivers
- ✅ **OLED Display Support** - Shows sensor data on 128x64 display
- ✅ **Error Handling** - Graceful handling of network failures
- ✅ **Production Ready** - Easy to deploy on Render, Railway, or Vercel

## 📁 Project Structure

```
task2/
├── backend/
│   ├── server.js           ← Main backend (relay + data distribution)
│   ├── package.json        ← Dependencies
│   └── .env.example        ← API key configuration
├── dashboard/
│   └── index.html          ← Minimal dashboard UI
├── esp32/
│   └── esp32_data_receiver.ino  ← Code for receiving ESP32
├── .gitignore
├── README.md
└── SETUP.md
```

## 🚀 Quick Start (Local Testing)

### 1️⃣ Install Backend Dependencies
```bash
cd backend
npm install
```

### 2️⃣ Configure Environment
```bash
# Copy example env file
copy .env.example .env

# Edit .env and set your API key
# Or use the default: your-secret-api-key-12345
```

### 3️⃣ Update Primary Backend URL in server.js
```javascript
const SOURCE_BACKEND_URL = 'https://test-fpbw.onrender.com/api/latest-data';
```

### 4️⃣ Start Backend
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
```

### 5️⃣ Open Dashboard
- Open browser: **http://localhost:3001**
- Dashboard will show live data from primary backend

### 6️⃣ Register Receiving ESP32
The ESP32 automatically registers when it starts and posts data to port 8080

## 📱 API Endpoints

### **GET /**
Returns API information
```bash
curl http://localhost:3001/
```

### **GET /api/latest-data**
Get latest sensor data
```bash
curl http://localhost:3001/api/latest-data
```

Response:
```json
{
  "status": "success",
  "message": "Latest data from source backend",
  "data": {
    "temperature": 25.3,
    "heartRate": 72,
    "motion": true,
    "distanceButton": false
  },
  "timestamp": "2026-02-07T10:30:45.123Z"
}
```

### **POST /api/register-esp**
Register a new receiving ESP32 (requires API key)
```bash
curl -X POST http://localhost:3001/api/register-esp \
  -H "Content-Type: application/json" \
  -d '{"apiKey":"your-secret-api-key-12345","espUrl":"http://192.168.1.100:8080"}'
```

### **POST /api/send-data**
Manually send data to all registered ESP32s (requires API key)
```bash
curl -X POST http://localhost:3001/api/send-data \
  -H "Content-Type: application/json" \
  -d '{"apiKey":"your-secret-api-key-12345","data":{"temperature":25.3,"heartRate":72,"motion":true,"distanceButton":false}}'
```

### **GET /api/health**
Health check
```bash
curl http://localhost:3001/api/health
```

## 🔑 API Key

**Default API Key:** `your-secret-api-key-12345`

⚠️ **IMPORTANT:** Change this in production!

Edit `.env` file:
```
API_KEY=my-super-secret-key-change-this
```

## 📤 Deploying to Render

### 1. Create GitHub Repository
```bash
cd task2
git init
git add .
git commit -m "Initial commit - IoT Relay Backend"
git push -u origin main
```

### 2. Deploy to Render
1. Go to https://render.com
2. Click "New +" → "Web Service"
3. Connect your GitHub repository
4. Fill in:
   - **Build Command:** `npm install && cd backend`
   - **Start Command:** `npm start`
   - **Environment Variables:**
     - `API_KEY`: your-unique-secret-key
5. Click "Create Web Service"

### 3. Get Your URL
Render will provide: `https://your-relay-app.onrender.com`

### 4. Update Source Backend URL in server.js
```javascript
const SOURCE_BACKEND_URL = 'https://your-relay-app.onrender.com/api/latest-data';
```

## 📝 Configuration for Receiving ESP32

Update these values in `esp32/esp32_data_receiver.ino`:

```cpp
// Line 27-28: WiFi Credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Line 33: Backend URL (after deployment)
const char* BACKEND_URL = "https://your-relay-app.onrender.com";

// Line 34: API Key (must match backend)
const char* API_KEY = "your-secret-api-key-12345";

// Line 35: Port to listen on
const int LISTEN_PORT = 8080;
```

## 🔌 OLED Display Wiring

Connect to ESP32:
- **SDA** → GPIO 21
- **SCL** → GPIO 22
- **GND** → GND
- **VCC** → 3.3V

## 📊 Data Flow Example

**Time: 10:30:00**
1. Primary ESP32 sends data → Primary Backend stores it
2. Relay Backend polls primary backend (GET /api/latest-data)
3. Relay Backend receives data → Updates in-memory storage
4. Relay Backend automatically broadcasts to all registered ESP32s (POST to port 8080)
5. Receiving ESP32 receives data → Updates OLED display
6. Dashboard fetches latest data → Displays real-time values

**Total Latency:** ~2 seconds from primary sensor to OLED display

## 🐛 Troubleshooting

### Dashboard shows "Waiting for data..."
- Ensure primary backend (test-fpbw.onrender.com) is running
- Check network connectivity
- Verify `SOURCE_BACKEND_URL` is correct

### Receiving ESP32 doesn't get data
- Check WiFi connection
- Verify API key matches in server.js and esp32 code
- Check ESP32 serial monitor (115200 baud)
- Ensure port 8080 is accessible

### Backend won't start
- Check Node.js is installed: `node --version`
- Install dependencies: `npm install`
- Check for port conflicts: `netstat -ano | findstr :3001`

## 📚 Dependencies

**Backend:**
- `express` - Web framework
- `cors` - Cross-origin support
- `axios` - HTTP client (polling)

**ESP32:**
- ArduinoJson - JSON parsing
- Adafruit SSD1306 - OLED library
- Adafruit GFX - Graphics library

## 📄 License

MIT

## 🆘 Support

Check [SETUP.md](SETUP.md) for detailed step-by-step instructions.
