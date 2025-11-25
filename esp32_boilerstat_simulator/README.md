# ESP32 BoilerStat Simulator

Arduino sketch that emulates the Python `mqtt_simulator.py` functionality on ESP32 hardware.

## Features
- Connects to WiFi network
- Publishes mock boiler sensor data via MQTT every 5 seconds
- JSON format identical to Python simulator
- Automatic reconnection for WiFi and MQTT
- NTP time synchronization
- Serial monitor output for debugging

## Required Libraries

Install these libraries through Arduino IDE Library Manager:

1. **PubSubClient** by Nick O'Leary (for MQTT)
2. **ArduinoJson** by Benoit Blanchon (for JSON formatting)
3. **NTPClient** by Fabrice Weinberg (for time sync)

Built-in libraries (no installation needed):
- WiFi (ESP32 Core)
- WiFiUdp (ESP32 Core)

## Configuration

Edit these values in the sketch before uploading:

```cpp
// WiFi Configuration
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// MQTT Configuration
const char* MQTT_BROKER = "192.168.1.245";  // Your MQTT broker IP
```

## Data Format

Publishes JSON messages to topic `boilerstat/reading`:

```json
{
  "timestamp": "2025-11-19 03:23:08",
  "boiler_state": 1,
  "zone_1": 0,
  "zone_2": 1,
  "zone_3": 1,
  "zone_4": 0,
  "zone_5": 1,
  "zone_6": 0
}
```

## Upload Instructions

1. Connect ESP32 to computer via USB
2. Open Arduino IDE
3. Select ESP32 board type (e.g., "ESP32 Dev Module")
4. Select correct COM port
5. Install required libraries
6. Update WiFi credentials in sketch
7. Upload sketch
8. Open Serial Monitor (115200 baud) to see output

## Serial Monitor Output

```
ESP32 BoilerStat Simulator Starting...
=====================================
Connecting to WiFi network: YourNetwork
WiFi connected successfully!
IP address: 192.168.1.100

MQTT broker configured: 192.168.1.245:1883
Publishing to topic: boilerstat/reading
Connecting to MQTT broker... connected!

[1] Published at 2025-11-19 03:23:08
    Boiler: 1 | Zones: 0 1 1 0 1 0
    JSON: {"timestamp":"2025-11-19 03:23:08","boiler_state":1,"zone_1":0,"zone_2":1,"zone_3":1,"zone_4":0,"zone_5":1,"zone_6":0}

[2] Published at 2025-11-19 03:23:13
    Boiler: 0 | Zones: 1 0 1 1 0 1
...
```

## Troubleshooting

- **WiFi connection fails**: Check SSID/password, ensure 2.4GHz network
- **MQTT connection fails**: Verify broker IP and port, check network connectivity
- **Time shows wrong**: NTP sync may take a few seconds, check internet connection
- **Upload fails**: Check USB cable, select correct board/port, press BOOT button if needed