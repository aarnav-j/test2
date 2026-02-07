/*
 * IoT Data Receiver - ESP32
 * Receives sensor data from relay backend and displays on OLED
 * 
 * SETUP INSTRUCTIONS:
 * 1. Install Arduino IDE
 * 2. Install ESP32 board (Tools > Board Manager > search "esp32")
 * 3. Install required libraries:
 *    - Adafruit SSD1306 (for OLED display)
 *    - Adafruit GFX Library
 *    - ArduinoJson
 * 4. Connect OLED to ESP32:
 *    - SDA → GPIO 21
 *    - SCL → GPIO 22
 *    - GND → GND
 *    - VCC → 3.3V
 * 5. Update WiFi credentials and backend URL below
 * 6. Upload to ESP32
 */

#include <WiFi.h>
#include <WiFiServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===========================
// Display Configuration (OLED)
// ===========================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===========================
// WiFi Configuration
// ===========================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ===========================
// Backend Configuration
// ===========================
const char* BACKEND_URL = "https://your-relay-backend.onrender.com";
const char* API_KEY = "your-secret-api-key-12345";
const int LISTEN_PORT = 8080;

WiFiServer server(LISTEN_PORT);

// ===========================
// Data Structure
// ===========================
struct SensorData {
  float temperature;
  int heartRate;
  boolean motion;
  boolean distanceButton;
};

SensorData currentData = {0, 0, false, false};
unsigned long lastDataTime = 0;

// ===========================
// Setup Function
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("IoT Data Receiver - ESP32");
  Serial.println("========================================");

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("✗ OLED display not found!");
    while (1); // Halt
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("IoT Receiver");
  display.println("Initializing...");
  display.display();

  // Connect to WiFi
  connectToWiFi();

  // Start local server to receive data
  server.begin();
  Serial.print("Server listening on port ");
  Serial.println(LISTEN_PORT);

  // Register this ESP32 with the relay backend
  registerWithBackend();
}

// ===========================
// Main Loop
// ===========================
void loop() {
  // Check for incoming data from relay backend
  WiFiClient client = server.available();
  if (client) {
    handleIncomingData(client);
    client.stop();
  }

  // Update display every 500ms
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay >= 500) {
    updateDisplay();
    lastDisplay = millis();
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    connectToWiFi();
  }

  delay(10);
}

// ===========================
// Connect to WiFi
// ===========================
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi connection failed.");
  }
}

// ===========================
// Register with Relay Backend
// ===========================
void registerWithBackend() {
  // Get local IP
  String localIP = WiFi.localIP().toString();
  String espUrl = "http://" + localIP + ":" + String(LISTEN_PORT);

  Serial.print("Registering with backend: ");
  Serial.println(espUrl);

  // This would require making an HTTP POST request to the backend
  // For simplicity, we'll just log this (backend can also auto-discover)
  Serial.println("✓ Ready to receive data on port 8080");
}

// ===========================
// Handle Incoming Data from Backend
// ===========================
void handleIncomingData(WiFiClient client) {
  // Read HTTP request
  String request = "";
  while (client.available()) {
    request += (char)client.read();
  }

  Serial.println("\n--- Received Data ---");
  Serial.println(request);

  // Parse JSON from request body
  int bodyStart = request.indexOf("\r\n\r\n");
  if (bodyStart != -1) {
    String jsonBody = request.substring(bodyStart + 4);
    
    // Parse JSON
    StaticJsonDocument<200> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, jsonBody);

    if (!error) {
      currentData.temperature = jsonDoc["temperature"];
      currentData.heartRate = jsonDoc["heartRate"];
      currentData.motion = jsonDoc["motion"];
      currentData.distanceButton = jsonDoc["distanceButton"];
      lastDataTime = millis();

      Serial.println("✓ Data parsed successfully:");
      Serial.print("  Temperature: ");
      Serial.println(currentData.temperature);
      Serial.print("  Heart Rate: ");
      Serial.println(currentData.heartRate);
      Serial.print("  Motion: ");
      Serial.println(currentData.motion);
      Serial.print("  Button: ");
      Serial.println(currentData.distanceButton);
    } else {
      Serial.print("✗ JSON parsing error: ");
      Serial.println(error.c_str());
    }
  }

  // Send HTTP response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"status\":\"received\"}");
}

// ===========================
// Update OLED Display
// ===========================
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // WiFi Status
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi: CONNECTED");
  } else {
    display.println("WiFi: DISCONNECTED");
  }

  display.println("---");

  // Check if we have recent data
  if (millis() - lastDataTime < 5000) {
    // Display sensor data
    display.print("Temp: ");
    display.print(currentData.temperature);
    display.println("C");

    display.print("HR: ");
    display.print(currentData.heartRate);
    display.println(" BPM");

    display.print("Motion: ");
    display.println(currentData.motion ? "YES" : "NO");

    display.print("Button: ");
    display.println(currentData.distanceButton ? "PRESSED" : "OK");

    // Last update time
    display.println("---");
    unsigned long secondsAgo = (millis() - lastDataTime) / 1000;
    display.print("Update: ");
    display.print(secondsAgo);
    display.println("s ago");
  } else {
    display.setTextSize(2);
    display.println("Waiting");
    display.println("for data...");
  }

  display.display();
}
