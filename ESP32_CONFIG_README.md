# ESP32 Configuration Setup

## Overview
The ESP32 code uses a configuration header file to manage credentials securely.

## Setup Instructions

1. **Copy the template file:**
   ```bash
   cp config.h.example config.h
   ```

2. **Edit the config.h file with your actual credentials:**
   ```c
   #define WIFI_SSID "Your-Actual-WiFi-Name"
   #define WIFI_PASSWORD "Your-Actual-WiFi-Password"
   #define MQTT_BROKER_IP "192.168.1.XXX"  // Your MQTT broker IP
   ```

3. **The config.h file is excluded from git** to protect your credentials.

## Files

- `config.h.example` - Template with placeholders (committed to git)
- `config.h` - Your actual credentials (excluded from git)
- `.gitignore` - Contains `config.h` to exclude it from version control

## Security Notes

- Never commit the actual `config.h` file with real credentials
- Always use the template approach for sharing code
- The `.gitignore` file protects your `config.h` from accidental commits