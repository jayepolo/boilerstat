/*
 * ESP32 Configuration Template
 * Copy this file and update with your actual credentials
 */

#ifndef ESP32_CONFIG_H
#define ESP32_CONFIG_H

// WiFi Configuration - Update with your network details
#define WIFI_SSID "Your-WiFi-Network-Name"
#define WIFI_PASSWORD "Your-WiFi-Password"

// MQTT Configuration - Update with your MQTT broker details
#define MQTT_BROKER_IP "192.168.1.xxx"  // Your MQTT broker IP
#define MQTT_BROKER_URI "mqtt://192.168.1.xxx:1883"
#define MQTT_PORT 1883

// MQTT Topics
#define MQTT_TOPIC "boilerstat/reading"
#define MQTT_CONTROL_TOPIC "boilerstat/control"

#endif // ESP32_CONFIG_H