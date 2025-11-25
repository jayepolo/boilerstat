/*
 * ESP32 BoilerStat Simulator
 * 
 * Emulates boiler sensor readings and publishes them via MQTT
 * Compatible with the Python mqtt_simulator.py data format
 * 
 * Libraries Required:
 * - WiFi (ESP32 Core)
 * - PubSubClient (MQTT)
 * - ArduinoJson
 * - NTPClient (for timestamps)
 * - WiFiUdp (for NTP)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"

// Configuration loaded from config.h
// All configuration constants defined in config.h

// Global Objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC, update every minute

// State Variables
unsigned long lastPublish = 0;
int publishCount = 1;
bool wifiConnected = false;
bool mqttConnected = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("ESP32 BoilerStat Simulator Starting...");
  Serial.println("=====================================");
  
  // Initialize WiFi
  setupWiFi();
  
  // Initialize MQTT
  setupMQTT();
  
  // Initialize NTP
  setupNTP();
  
  Serial.println("Setup complete. Starting main loop...");
  Serial.println();
}

void loop() {
  // Maintain connections
  maintainConnections();
  
  // Update NTP time
  timeClient.update();
  
  // Publish data at specified interval
  if (millis() - lastPublish >= PUBLISH_INTERVAL_MS) {
    if (mqttConnected) {
      publishSensorData();
      lastPublish = millis();
    } else {
      Serial.println("MQTT not connected - skipping publish");
    }
  }
  
  // Handle MQTT client loop
  mqttClient.loop();
  
  delay(100); // Small delay to prevent watchdog issues
}

// WiFi Setup and Management
void setupWiFi() {
  Serial.print("Connecting to WiFi network: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("WiFi connection failed!");
  }
  Serial.println();
}

void maintainConnections() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("WiFi connection lost - attempting reconnection...");
      wifiConnected = false;
      mqttConnected = false;
    }
    setupWiFi();
  }
  
  // Check MQTT connection
  if (wifiConnected && !mqttClient.connected()) {
    if (mqttConnected) {
      Serial.println("MQTT connection lost - attempting reconnection...");
      mqttConnected = false;
    }
    connectToMQTT();
  }
}

// MQTT Setup and Management
void setupMQTT() {
  mqttClient.setServer(MQTT_BROKER_IP, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  
  Serial.print("MQTT broker configured: ");
  Serial.print(MQTT_BROKER_IP);
  Serial.print(":");
  Serial.println(MQTT_PORT);
  Serial.print("Publishing to topic: ");
  Serial.println(MQTT_TOPIC);
  Serial.println();
}

void connectToMQTT() {
  if (!wifiConnected) {
    return;
  }
  
  Serial.print("Connecting to MQTT broker...");
  
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      mqttConnected = true;
      Serial.println(" connected!");
      Serial.print("Client ID: ");
      Serial.println(MQTT_CLIENT_ID);
    } else {
      Serial.print(".");
      attempts++;
      delay(1000);
    }
  }
  
  if (!mqttConnected) {
    Serial.println(" failed!");
    Serial.print("MQTT state: ");
    Serial.println(mqttClient.state());
  }
  Serial.println();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Callback for received MQTT messages (not used in this simulator)
  // But required by PubSubClient
}

// NTP Setup
void setupNTP() {
  Serial.println("Initializing NTP client...");
  timeClient.begin();
  timeClient.update();
  
  Serial.print("Current time: ");
  Serial.println(getFormattedTime());
  Serial.println();
}

// Time Formatting
String getFormattedTime() {
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  
  char timeBuffer[20];
  sprintf(timeBuffer, "%04d-%02d-%02d %02d:%02d:%02d",
          ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
          ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  
  return String(timeBuffer);
}

// Sensor Data Generation and Publishing
void publishSensorData() {
  // Generate sensor readings with targeted utilization rates
  DynamicJsonDocument doc(256);
  
  doc["timestamp"] = getFormattedTime();
  
  // Target utilization rates: Zone N = N * 10% (Zone 1=10%, Zone 2=20%, etc.)
  // Burner target: 50% utilization
  float zone_target_utilization[6] = {0.10, 0.20, 0.30, 0.40, 0.50, 0.60};
  float burner_target_utilization = 0.50;
  
  // Generate zone states based on target utilization with ±20% variation
  int zones[6];
  for (int i = 0; i < 6; i++) {
    float target_rate = zone_target_utilization[i];
    
    // Add ±20% variation (of the target rate itself)
    float variation_range = 0.20;
    float variation = (random(0, 1000) / 1000.0 - 0.5) * 2.0 * (target_rate * variation_range);
    float adjusted_rate = target_rate + variation;
    
    // Ensure rate stays within bounds
    if (adjusted_rate < 0.0) adjusted_rate = 0.0;
    if (adjusted_rate > 1.0) adjusted_rate = 1.0;
    
    // Generate random value and compare to adjusted rate
    zones[i] = (random(0, 1000) / 1000.0 < adjusted_rate) ? 1 : 0;
  }
  
  // Generate burner state with 50% target ±30% variation
  float burner_variation_range = 0.30;
  float burner_variation = (random(0, 1000) / 1000.0 - 0.5) * 2.0 * (burner_target_utilization * burner_variation_range);
  float adjusted_burner_rate = burner_target_utilization + burner_variation;
  if (adjusted_burner_rate < 0.0) adjusted_burner_rate = 0.0;
  if (adjusted_burner_rate > 1.0) adjusted_burner_rate = 1.0;
  
  int burner = (random(0, 1000) / 1000.0 < adjusted_burner_rate) ? 1 : 0;
  
  // Light correlation - if many zones calling, burner slightly more likely on
  int active_zones = 0;
  for (int i = 0; i < 6; i++) {
    if (zones[i]) active_zones++;
  }
  if (active_zones >= 3 && burner == 0 && random(0, 100) < 30) {
    burner = 1;
  }
  
  // Set the JSON values
  doc["boiler_state"] = burner;
  doc["zone_1"] = zones[0];
  doc["zone_2"] = zones[1];
  doc["zone_3"] = zones[2];
  doc["zone_4"] = zones[3];
  doc["zone_5"] = zones[4];
  doc["zone_6"] = zones[5];
  
  // Convert to JSON string
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Publish to MQTT
  bool success = mqttClient.publish(MQTT_TOPIC, jsonString.c_str());
  
  if (success) {
    Serial.printf("[%d] Published at %s\n", publishCount, doc["timestamp"].as<const char*>());
    Serial.printf("    Boiler: %d | Zones: %d %d %d %d %d %d\n", 
                  doc["boiler_state"].as<int>(),
                  doc["zone_1"].as<int>(), doc["zone_2"].as<int>(), doc["zone_3"].as<int>(),
                  doc["zone_4"].as<int>(), doc["zone_5"].as<int>(), doc["zone_6"].as<int>());
    Serial.printf("    JSON: %s\n", jsonString.c_str());
    publishCount++;
  } else {
    Serial.printf("[%d] Publish failed!\n", publishCount);
  }
  Serial.println();
}