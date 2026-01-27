#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// ⚠️ IMPORTANT: Replace these with your Wi-Fi and AWS credentials ⚠️
const char* WIFI_SSID = "Sky-WiFi-FE01";
const char* WIFI_PASSWORD = "fingeruncle9628";

const char* AWS_IOT_ENDPOINT = "a1zx1lood4k4kc-ats.iot.ap-southeast-2.amazonaws.com";
const char* AWS_THING_NAME = "ESP32-S3";

// Define MQTT topics
#define AWS_IOT_PUBLISH_TOPIC "esp32/data"
#define AWS_IOT_LOG_TOPIC "esp32/logs"

// Create secure Wi-Fi and MQTT clients
WiFiClientSecure net;
PubSubClient client(net);

// Pin definitions
Adafruit_NeoPixel LED_RGB(1, 48, NEO_GRBW + NEO_KHZ800);
#define SOIL_MOISTURE_PIN 4  // Soil sensor A0 → GPIO 4
#define DHT_PIN 5            // DHT22 data → GPIO 5
#define RELAY_PIN 6          // Relay (water pump) → GPIO 6
#define DHT_TYPE DHT22

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Soil sensor calibration values
const int DRY_VALUE = 4095;   // Value in dry air
const int WET_VALUE = 1120;   // Value with wet tissue

// Watering system settings
const int WATER_THRESHOLD_LOW = 25;   // Start watering when below 25%
const int WATER_THRESHOLD_HIGH = 60;  // Stop watering when above 60%
const unsigned long MAX_WATERING_TIME = 2000;  // Max 2 seconds for small cup
const unsigned long MIN_WAIT_TIME = 10000;     // Wait 10 seconds between cycles (demo friendly)

// --- Security Enhancements ---
// 1. Sensor Data Validation Ranges
const float MIN_TEMPERATURE_C = 0.0;
const float MAX_TEMPERATURE_C = 40.0;
const float MIN_HUMIDITY_PERCENT = 30.0;
const float MAX_HUMIDITY_PERCENT = 90.0;
const int MIN_MOISTURE_PERCENT = 10;
const int MAX_MOISTURE_PERCENT = 90;

// 2. Tamper Detection: Max change per second
const float MAX_TEMP_CHANGE_PER_SEC = 5.0; // Max 5°C change per second
const float MAX_HUMIDITY_CHANGE_PER_SEC = 10.0; // Max 10% change per second
const float MAX_MOISTURE_CHANGE_PER_SEC = 20.0; // Max 20% change per second

// Variables to store last valid sensor readings for tamper detection
float last_valid_temperature = -999.0;
float last_valid_humidity = -999.0;
int last_valid_moisture = -1;
unsigned long last_sensor_check_time = 0;
// --- End Security Enhancements ---

// Watering system variables
bool isWatering = false;
unsigned long wateringStartTime = 0;
unsigned long lastWateringTime = 0;
unsigned long lastPublishTime = 0;
const long PUBLISH_INTERVAL = 2000; // Rate Limiting: No more than 1 message per 2 seconds

// Function to publish security logs to a separate MQTT topic
void publishLog(const char* level, const char* message) {
  StaticJsonDocument<200> doc;
  doc["level"] = level;
  doc["message"] = message;
  doc["thingName"] = AWS_THING_NAME;
  doc["timestamp"] = millis();

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  if (client.connected()) {
    client.publish(AWS_IOT_LOG_TOPIC, jsonBuffer);
    Serial.print("LOG PUBLISHED: ");
    Serial.println(message);
  }
}

// Sensor Data Validation and Tamper Detection
bool validateSensorData(int& moisture, float& temp, float& hum) {
  unsigned long currentTime = millis();
  float timeDeltaSec = (currentTime - last_sensor_check_time) / 1000.0;
  
  // Reset initial values on first run
  if (last_sensor_check_time == 0) {
      last_valid_temperature = temp;
      last_valid_humidity = hum;
      last_valid_moisture = moisture;
      timeDeltaSec = 0; // Avoid division by zero
  }

  // 1. Check for valid ranges
  if (isnan(temp) || temp < MIN_TEMPERATURE_C || temp > MAX_TEMPERATURE_C) {
    publishLog("ALERT", "Temperature out of range or NaN.");
    temp = last_valid_temperature; // Revert to last known good value
    return false;
  }
  if (isnan(hum) || hum < MIN_HUMIDITY_PERCENT || hum > MAX_HUMIDITY_PERCENT) {
    publishLog("ALERT", "Humidity out of range or NaN.");
    hum = last_valid_humidity; // Revert
    return false;
  }
   if (moisture < MIN_MOISTURE_PERCENT || moisture > MAX_MOISTURE_PERCENT) {
    publishLog("ALERT", "Moisture out of range.");
    moisture = last_valid_moisture; // Revert
    return false;
  }

  // 2. Tamper detection: Check for sudden changes if time has passed
  if (timeDeltaSec > 0.1) { // Only check if some time has passed
      float tempChange = abs(temp - last_valid_temperature) / timeDeltaSec;
      if (last_valid_temperature > -998.0 && tempChange > MAX_TEMP_CHANGE_PER_SEC) {
          publishLog("ALERT", "Tampering suspected: Rapid temperature change.");
          return false;
      }

      float humidityChange = abs(hum - last_valid_humidity) / timeDeltaSec;
      if (last_valid_humidity > -998.0 && humidityChange > MAX_HUMIDITY_CHANGE_PER_SEC) {
          publishLog("ALERT", "Tampering suspected: Rapid humidity change.");
          return false;
      }

       int moistureChange = abs(moisture - last_valid_moisture) / timeDeltaSec;
      if (last_valid_moisture != -1 && moistureChange > MAX_MOISTURE_CHANGE_PER_SEC) {
          publishLog("ALERT", "Tampering suspected: Rapid moisture change.");
          return false;
      }
  }

  // If all checks pass, update last valid values
  last_valid_temperature = temp;
  last_valid_humidity = hum;
  last_valid_moisture = moisture;
  last_sensor_check_time = currentTime;
  
  return true;
}

// 5. Placeholder for Secure Firmware Update
void secureFirmwareUpdate() {
  // This is a placeholder. A real implementation would:
  // 1. Periodically check a secure server (e.g., via HTTPS) for a manifest file.
  // 2. The manifest would contain the new firmware version and a cryptographic signature.
  // 3. If a new version exists, download the signed firmware binary.
  // 4. Verify the binary's signature using a public key stored on the device.
  // 5. If verification is successful, perform the OTA update.
  // 6. If verification fails, log a security alert.
  // Example: if (check_for_update_and_verify_signature()) { perform_ota(); }
}

// Function to read a file from LittleFS
String readFile(const char *path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading: " + String(path));
    return "";
  }
  String content = file.readString();
  file.close();
  return content;
}

// Function to connect to Wi-Fi
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to AWS IoT Core
void connectAWS() {
  // Read certificates from LittleFS
  String ca_str = readFile("/ca.pem");
  String cert_str = readFile("/cert.crt");
  String private_key_str = readFile("/private.key");

  // 3. Secure MQTT Communication (TLS/SSL on port 8883)
  // WiFiClientSecure handles the TLS encryption.
  net.setCACert(ca_str.c_str());
  net.setCertificate(cert_str.c_str());
  net.setPrivateKey(private_key_str.c_str());

  Serial.print("Attempting to connect to AWS IoT (securely)...");
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Attempt to connect and check for success
  if (client.connect(AWS_THING_NAME)) {
    Serial.println(" Connected!");
  } else {
    Serial.print(" Failed to connect. Error code: ");
    Serial.println(client.state());
    Serial.println("Retrying in 5 seconds...");
    delay(5000);
  }
}

// Function to publish sensor data to AWS IoT Core
void publishSensorData(int moisture, float temp, float hum) {
  StaticJsonDocument<200> doc;
  doc["moisture_percent"] = moisture;
  doc["temperature_c"] = temp;
  doc["humidity_percent"] = hum;
  doc["watering_active"] = isWatering;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  // Publish the message
  if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)) {
    Serial.println("Published successfully!");
  } else {
    Serial.println("Publish failed.");
  }
}

void startWatering() {
  isWatering = true;
  wateringStartTime = millis();
  digitalWrite(RELAY_PIN, HIGH); // Send HIGH signal to relay → Turn ON water pump
  Serial.println("💧 STARTING WATER PUMP! (3s max)");
  Serial.println("📶 Relay Signal: HIGH (Pump ON)");
}

void stopWatering() {
  isWatering = false;
  lastWateringTime = millis();
  digitalWrite(RELAY_PIN, LOW);  // Send LOW signal (no signal) to relay → Turn OFF water pump
  Serial.println("💧 STOPPING WATER PUMP");
  Serial.println("📶 Relay Signal: LOW (Pump OFF)");
  
  // Calculate watering duration
  float wateringDuration = (millis() - wateringStartTime) / 1000.0;
  Serial.println("💧 Watered for " + String(wateringDuration, 1) + " seconds");
  Serial.println("⏳ Next watering available in 10 seconds");
}

void checkAndControlWatering(int moisturePercent) {
  unsigned long currentTime = millis();
  
  // Safety check: Maximum watering time (3 seconds)
  if (isWatering && (currentTime - wateringStartTime) > MAX_WATERING_TIME) {
    stopWatering();
    Serial.println("*** ⏰ 3-second watering complete! ***");
    return;
  }
  
  // Check if we should start watering
  if (!isWatering && moisturePercent < WATER_THRESHOLD_LOW) {
    // Check if enough time has passed since last watering (10 seconds)
    if (currentTime - lastWateringTime > MIN_WAIT_TIME) {
      startWatering();
    } else {
      Serial.println("⏳ wait period... (" + String((MIN_WAIT_TIME - (currentTime - lastWateringTime))/1000) + "s remaining)");
    }
  }
  
  // Check if we should stop watering
  if (isWatering && moisturePercent >= WATER_THRESHOLD_HIGH) {
    stopWatering();
  }
}

void setLEDStatus(int moisturePercent) {
  if (isWatering) {
    // Blinking blue when watering (faster for demo)
    static unsigned long lastBlink = 0;
    static bool blinkState = false;
    if (millis() - lastBlink > 250) { // Faster blinking for demo
      blinkState = !blinkState;
      lastBlink = millis();
      if (blinkState) {
        LED_RGB.setPixelColor(0, LED_RGB.Color(0, 0, 255)); // Blue
      } else {
        LED_RGB.setPixelColor(0, LED_RGB.Color(0, 0, 0));   // Off
      }
    }
  } else if (moisturePercent < WATER_THRESHOLD_LOW) {
    LED_RGB.setPixelColor(0, LED_RGB.Color(255, 0, 0)); // Red - Dry
  } else if (moisturePercent < WATER_THRESHOLD_HIGH) {
    LED_RGB.setPixelColor(0, LED_RGB.Color(255, 255, 0)); // Yellow - Moderate
  } else {
    LED_RGB.setPixelColor(0, LED_RGB.Color(0, 255, 0)); // Green - Good
  }
  LED_RGB.show();
}

void setup() {
  Serial.begin(115200);
  LED_RGB.begin();
  LED_RGB.setBrightness(50);
  dht.begin();
  
  // Initialize relay pin (LOW = no signal = relay OFF)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Start with relay OFF (no signal)

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  connectWiFi();

  Serial.println("Smart Plant Watering System");
  Serial.println("Soil: GPIO 4 | DHT22: GPIO 5 | Relay: GPIO 6 | Port: COM9");
  Serial.println("Watering: Start <" + String(WATER_THRESHOLD_LOW) + "% | Stop >" + String(WATER_THRESHOLD_HIGH) + "%");
  Serial.println("Settings: 3s max watering | 10s wait time");
  Serial.println("===================");
}

void loop() {
  // Check for AWS connection and reconnect if necessary
  if (!client.connected()) {
    connectAWS();
  }
  client.loop(); // Keep the MQTT connection alive
  
  // Read sensor data
  int smsValue = analogRead(SOIL_MOISTURE_PIN);
  int moisturePercent = map(smsValue, DRY_VALUE, WET_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);
  
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Validate sensor data before acting on it or publishing
  if (validateSensorData(moisturePercent, temperature, humidity)) {
    // 4. Rate Limiting: Only publish periodically
    if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
      if (client.connected()) {
        publishSensorData(moisturePercent, temperature, humidity);
      }
      lastPublishTime = millis();
    }
    checkAndControlWatering(moisturePercent);
  }

  // LED color based on system status
  setLEDStatus(moisturePercent);

  // 5. Check for secure firmware updates periodically
  secureFirmwareUpdate();

  // Print all readings to Serial Monitor
  Serial.println("=== SMART WATERING SYSTEM ===");
  Serial.print("Soil - Raw: ");
  Serial.print(smsValue);
  Serial.print(" | Moisture: ");
  Serial.print(moisturePercent);
  Serial.print("% | Status: ");
  
  if (isWatering) {
    Serial.println("💧 WATERING ACTIVE!");
  } else if (moisturePercent < WATER_THRESHOLD_LOW) {
    Serial.println("🔴 DRY - Needs water!");
  } else if (moisturePercent < WATER_THRESHOLD_HIGH) {
    Serial.println("🟡 MODERATE");
  } else {
    Serial.println("🟢 WET - Good level");
  }
  
  Serial.print("💧 Water Pump: ");
  Serial.print(isWatering ? "ON" : "OFF");
  if (isWatering) {
    Serial.print(" (Running for ");
    Serial.print((millis() - wateringStartTime) / 1000.0, 1);
    Serial.print("s)");
  } else if ((millis() - lastWateringTime) < MIN_WAIT_TIME && lastWateringTime > 0) {
    Serial.print(" (Wait: ");
    Serial.print((MIN_WAIT_TIME - (millis() - lastWateringTime)) / 1000);
    Serial.print("s)");
  }
  Serial.println();
  
  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.print("🌡️ Air - Temp: ");
    Serial.print(temperature);
    Serial.print("°C | Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
  }
  
  Serial.println("-----------------------------");
  Serial.println();
  
  delay(2000); // Faster updates for demo
}