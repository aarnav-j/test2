/*
 * IoT Data Receiver - ESP32 with OLED Display & Buzzer
 * Receives sensor data from relay backend, displays on OLED, and controls Buzzer
 * 
 * PIN CONFIGURATION:
 * - OLED SCL → GPIO 22  (I2C Clock)
 * - OLED SDA → GPIO 21  (I2C Data)
 * - Buzzer (+) → GPIO 18 (PWM/Tone output)
 * - GND → GND
 * - VCC (3.3V) → 3V3
 * 
 * LIBRARIES REQUIRED:
 * - Adafruit SSD1306
 * - Adafruit GFX Library
 * - ArduinoJson
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
#define I2C_SDA 21
#define I2C_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TwoWire I2COne = TwoWire(0);

// ===========================
// Buzzer Configuration
// ===========================
#define BUZZER_PIN 18
#define BUZZER_CHANNEL 0
#define BUZZER_FREQUENCY 2000
#define BUZZER_RESOLUTION 8

// ===========================
// WiFi Configuration
// ===========================
const char* WIFI_SSID = "Room 003";
const char* WIFI_PASSWORD = "20060524";

// ===========================
// Backend Configuration
// ===========================
const char* BACKEND_URL = "https://test2-3-wu9g.onrender.com";
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
boolean isMotionActive = false;
boolean isBuzzerActive = false;

// ===========================
// Setup Function
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("IoT Data Receiver - ESP32");
  Serial.println("OLED + Buzzer Edition");
  Serial.println("========================================");

  // Initialize Buzzer
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQUENCY, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 0); // Start silent
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("✓ Buzzer initialized (GPIO 18)");

  // Initialize OLED Display with custom I2C pins
  I2COne.begin(I2C_SDA, I2C_SCL, 100000);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println("✗ OLED display not found!");
    Serial.println("  Check wiring: SDA=GPIO21, SCL=GPIO22, GND, 3V3");
    delay(2000);
  } else {
    Serial.println("✓ OLED display initialized (GPIO 21/22)");
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("IoT Receiver");
  display.println("Initializing...");
  display.display();
  delay(1000);

  // Connect to WiFi
  connectToWiFi();

  // Start local server to receive data
  server.begin();
  Serial.print("✓ Server listening on port ");
  Serial.println(LISTEN_PORT);

  // Register this ESP32 with the relay backend
  registerWithBackend();
  
  // Display ready message
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Ready!");
  display.println("Waiting for data...");
  display.display();
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
    
    // Trigger buzzer on data received
    triggerBuzzer(100, 1000); // 100ms beep at 1000Hz
  }

  // Update display every 500ms
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay >= 500) {
    updateDisplay();
    lastDisplay = millis();
  }

  // Control buzzer based on motion/data
  static unsigned long lastBuzzerCheck = 0;
  if (millis() - lastBuzzerCheck >= 200) {
    updateBuzzerState();
    lastBuzzerCheck = millis();
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ WiFi disconnected. Attempting to reconnect...");
    connectToWiFi();
  }

  delay(10);
}

// ===========================
// Buzzer Control - Trigger beep
// ===========================
void triggerBuzzer(int duration, int frequency) {
  ledcWriteTone(BUZZER_CHANNEL, frequency);
  delay(duration);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

// ===========================
// Buzzer Control - Continuous tone
// ===========================
void startBuzzer(int frequency) {
  ledcWriteTone(BUZZER_CHANNEL, frequency);
  isBuzzerActive = true;
}

void stopBuzzer() {
  ledcWriteTone(BUZZER_CHANNEL, 0);
  isBuzzerActive = false;
}

// ===========================
// Update Buzzer State based on sensor data
// ===========================
void updateBuzzerState() {
  // Check if data is fresh (less than 5 seconds old)
  if (millis() - lastDataTime < 5000) {
    
    // Motion detected - continuous low tone
    if (currentData.motion) {
      if (!isMotionActive) {
        startBuzzer(800); // Low tone for motion
        isMotionActive = true;
      }
    } else {
      if (isMotionActive) {
        stopBuzzer();
        isMotionActive = false;
      }
    }
    
    // Button pressed - quick high beep
    if (currentData.distanceButton) {
      triggerBuzzer(50, 1500); // Quick high beep for button press
      delay(50);
    }
  } else {
    stopBuzzer();
    isMotionActive = false;
  }
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

  // Header with WiFi status
  display.print("IoT Receiver  ");
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi:");
    display.println("CONN");
  } else {
    display.println("WiFi:");
    display.println("DOWN");
  }

  display.println("================");

  // Check if we have recent data
  unsigned long timeSinceData = millis() - lastDataTime;
  
  if (timeSinceData < 5000) {
    // Display sensor data
    display.print("TEMP: ");
    display.print(currentData.temperature, 1);
    display.println("C");

    display.print("HR: ");
    display.print(currentData.heartRate);
    display.println(" bpm");

    display.print("Motion: ");
    display.println(currentData.motion ? "[YES]" : "[NO]");

    display.print("Button: ");
    display.println(currentData.distanceButton ? "[ON]" : "[OFF]");

    display.println("----------------");
    display.print("Updated ");
    display.print(timeSinceData / 1000);
    display.println("s ago");
  } else {
    display.setTextSize(2);
    display.println("  WAITING");
    display.println("  FOR");
    display.println("  DATA...");
  }

  display.display();
}
