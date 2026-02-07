const express = require('express');
const cors = require('cors');
const axios = require('axios');
const path = require('path');

const app = express();

// Middleware
app.use(cors());
app.use(express.json());

// ===========================
// Configuration
// ===========================
const API_KEY = process.env.API_KEY || 'your-secret-api-key-12345';
const SOURCE_BACKEND_URL = 'https://test-fpbw.onrender.com/api/latest-data';
const POLL_INTERVAL = 2000; // Poll every 2 seconds

// ===========================
// In-Memory Storage
// ===========================
let latestData = {
  temperature: 0,
  pulseRate: 0,
  distress: false,
  rfid: false,
  ir: false
};
let lastUpdateTime = null;
let receiverESPUrls = []; // Store ESP32 receiver URLs

// ===========================
// ENDPOINT 1: GET /
// Root Route - API Info
// ===========================
app.get('/', (req, res) => {
  res.json({
    status: 'ok',
    message: 'IoT Data Relay Backend is running',
    endpoints: {
      'GET /api/latest-data': 'Get latest sensor data',
      'POST /api/register-esp': 'Register ESP32 receiver URL',
      'POST /api/send-data': 'Send data to registered ESP32 devices',
      'GET /api/health': 'Health check'
    }
  });
});

// ===========================
// ENDPOINT 2: GET /api/latest-data
// Returns the latest data received from source backend
// ===========================
app.get('/api/latest-data', (req, res) => {
  res.json({
    status: 'success',
    message: 'Latest data from source backend',
    data: latestData,
    timestamp: lastUpdateTime
  });
});

// ===========================
// ENDPOINT 3: POST /api/register-esp
// Register ESP32 receiver URL (requires API key)
// ===========================
app.post('/api/register-esp', (req, res) => {
  const { apiKey, espUrl } = req.body;

  // Validate API key
  if (apiKey !== API_KEY) {
    return res.status(401).json({
      error: 'Unauthorized: Invalid API key'
    });
  }

  if (!espUrl) {
    return res.status(400).json({
      error: 'Missing espUrl in request body'
    });
  }

  // Add URL to list if not already present
  if (!receiverESPUrls.includes(espUrl)) {
    receiverESPUrls.push(espUrl);
    console.log(`✓ ESP32 registered: ${espUrl}`);
  }

  res.json({
    status: 'success',
    message: 'ESP32 registered successfully',
    registeredESPs: receiverESPUrls.length
  });
});

// ===========================
// ENDPOINT 4: POST /api/send-data
// Send data to all registered ESP32 devices (requires API key)
// ===========================
app.post('/api/send-data', async (req, res) => {
  const { apiKey, data } = req.body;

  // Validate API key
  if (apiKey !== API_KEY) {
    return res.status(401).json({
      error: 'Unauthorized: Invalid API key'
    });
  }

  if (!data) {
    return res.status(400).json({
      error: 'Missing data in request body'
    });
  }

  // Send data to all registered ESP32 devices
  let successCount = 0;
  let failCount = 0;

  for (const espUrl of receiverESPUrls) {
    try {
      await axios.post(espUrl, data, {
        timeout: 5000,
        headers: { 'Content-Type': 'application/json' }
      });
      successCount++;
      console.log(`✓ Data sent to: ${espUrl}`);
    } catch (error) {
      failCount++;
      console.error(`✗ Failed to send to ${espUrl}: ${error.message}`);
    }
  }

  res.json({
    status: 'success',
    message: 'Broadcast complete',
    sent: successCount,
    failed: failCount,
    totalESPs: receiverESPUrls.length
  });
});

// ===========================
// ENDPOINT 5: GET /api/health
// Health check
// ===========================
app.get('/api/health', (req, res) => {
  res.json({
    status: 'ok',
    message: 'Relay backend is running',
    uptime: process.uptime(),
    registeredESPs: receiverESPUrls.length,
    lastDataUpdate: lastUpdateTime
  });
});

// ===========================
// Background Task: Poll Source Backend
// Runs every 2 seconds to fetch data from primary backend
// ===========================
setInterval(async () => {
  try {
    const response = await axios.get(SOURCE_BACKEND_URL, { timeout: 5000 });
    
    if (response.data && response.data.data) {
      // Extract and map all 5 parameters
      const sourceData = response.data.data;
      
      latestData = {
        temperature: sourceData.temperature || 0,
        pulseRate: sourceData.pulseRate || sourceData.pulse_rate || 0,
        distress: sourceData.distress || false,
        rfid: sourceData.rfid || false,
        ir: sourceData.ir || false
      };
      
      lastUpdateTime = new Date().toISOString();
      console.log(`[${lastUpdateTime}] Data fetched from source:`, latestData);

      // Automatically broadcast to all registered ESP32s
      if (receiverESPUrls.length > 0) {
        broadcastToESP32s(latestData);
      }
    }
  } catch (error) {
    console.error('Error polling source backend:', error.message);
  }
}, POLL_INTERVAL);

// ===========================
// Helper Function: Broadcast to ESP32s
// ===========================
async function broadcastToESP32s(data) {
  for (const espUrl of receiverESPUrls) {
    try {
      await axios.post(espUrl, data, {
        timeout: 5000,
        headers: { 'Content-Type': 'application/json' }
      });
    } catch (error) {
      console.error(`Failed to send to ${espUrl}:`, error.message);
    }
  }
}

// ===========================
// Serve Dashboard
// ===========================
app.use(express.static(path.join(__dirname, '../dashboard')));

// ===========================
// Error Handling
// ===========================
app.use((err, req, res, next) => {
  console.error('Unhandled error:', err);
  res.status(500).json({
    error: 'Internal server error'
  });
});

// ===========================
// Start Server
// ===========================
const PORT = process.env.PORT || 3001;

app.listen(PORT, () => {
  console.log(`\n✓ IoT Data Relay Backend running on http://localhost:${PORT}`);
  console.log(`\nConfiguration:`);
  console.log(`  Source Backend: ${SOURCE_BACKEND_URL}`);
  console.log(`  Poll Interval: ${POLL_INTERVAL}ms`);
  console.log(`  API Key: ${API_KEY === 'your-secret-api-key-12345' ? '⚠️ DEFAULT (CHANGE THIS!)' : '✓ Set'}`);
  console.log(`\nEndpoints:`);
  console.log(`  GET  /api/latest-data`);
  console.log(`  POST /api/register-esp (requires API key)`);
  console.log(`  POST /api/send-data (requires API key)`);
  console.log(`  GET  /api/health\n`);
});
