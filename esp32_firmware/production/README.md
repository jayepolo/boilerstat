# ESP32 BoilerStat Simulator - ESP-IDF Project

Native ESP-IDF C implementation that emulates the Python `mqtt_simulator.py` functionality on ESP32 hardware.

## Features
- **Native ESP-IDF implementation** - No Arduino framework dependencies
- **WiFi connection** with automatic reconnection
- **MQTT publishing** every 5 seconds to `boilerstat/reading` topic
- **JSON message format** identical to Python simulator
- **SNTP time synchronization** for accurate timestamps
- **Visual LED feedback** - flashes to indicate WiFi connection and MQTT publishes
- **Robust error handling** and reconnection logic
- **FreeRTOS task-based architecture** for reliability

## Project Structure
```
boilerstat_simulator/
├── CMakeLists.txt              # Main project CMake file
├── sdkconfig.defaults          # Default project configuration
├── README.md                   # This file
└── main/
    ├── CMakeLists.txt          # Main component CMake file
    ├── idf_component.yml       # Component dependencies
    └── main.c                  # Main application code
```

## Prerequisites
- ESP-IDF development environment (Docker container setup)
- ESP32 development board
- WiFi network access

## Configuration
WiFi credentials and LED settings are hardcoded in `main/main.c`:
```c
#define WIFI_SSID "Apponaug-WiFi"
#define WIFI_PASSWORD "Ollie+Millie"
#define MQTT_BROKER_URI "mqtt://192.168.1.245:1883"
#define LED_GPIO_PIN GPIO_NUM_2  // Built-in LED on most ESP32 boards
```

## LED Feedback Patterns
- **WiFi Connected**: LED flashes 10 times over 3 seconds
- **MQTT Publish**: LED flashes 3 times over 1 second (every 5 seconds)
- **Startup**: LED off during initialization

## Build and Flash Commands

### Start ESP-IDF Container
```bash
cd ~/esp/esp-idf-project
docker compose up -d
docker exec -it esp-idf /bin/bash
```

### Inside Container - Set up Environment
```bash
# Source ESP-IDF environment
. $IDF_PATH/export.sh

# Navigate to project
cd /project/boilerstat_simulator
```

### Build Project
```bash
# Set target (adjust for your ESP32 variant)
idf.py set-target esp32

# Build the project
idf.py build
```

### Flash to ESP32
```bash
# Check device connection
ls /dev/ttyUSB*

# Flash to device (adjust port if needed)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
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

## Expected Serial Output
```
I (1234) BOILERSTAT_SIM: ESP32 BoilerStat Simulator Starting...
I (1240) BOILERSTAT_SIM: =====================================
I (1245) BOILERSTAT_SIM: LED initialized on GPIO 2
I (2500) BOILERSTAT_SIM: Connected to WiFi network: Apponaug-WiFi
I (3200) BOILERSTAT_SIM: Got IP:192.168.1.100
I (3205) BOILERSTAT_SIM: WiFi connected - flashing LED 10 times
I (3800) BOILERSTAT_SIM: MQTT_EVENT_CONNECTED
I (4200) BOILERSTAT_SIM: Time synchronized
I (5000) BOILERSTAT_SIM: [1] Published at 2025-11-19 03:23:08
I (5005) BOILERSTAT_SIM:     Boiler: 1 | Zones: 0 1 1 0 1 0
I (5010) BOILERSTAT_SIM:     JSON: {"timestamp":"2025-11-19 03:23:08","boiler_state":1,"zone_1":0,"zone_2":1,"zone_3":1,"zone_4":0,"zone_5":1,"zone_6":0}
```

## Troubleshooting
- **Build errors**: Ensure ESP-IDF environment is sourced with `. $IDF_PATH/export.sh`
- **WiFi connection fails**: Check SSID/password, ensure 2.4GHz network
- **MQTT connection fails**: Verify broker IP and network connectivity
- **Upload fails**: Check USB connection, correct port, press BOOT button if needed
- **Time sync issues**: Ensure internet connectivity for SNTP
- **LED not working**: Check if your board has built-in LED on GPIO2, adjust LED_GPIO_PIN if needed

## Integration
This ESP32 program generates identical data to the Python simulator, allowing seamless integration with your existing boilerstat monitoring system. The JSON messages will be received by your dockerized MQTT listener and logged to the database.

## Next Steps
1. Replace mock sensor readings with actual GPIO inputs for real boiler monitoring
2. Add LCD display support for local status indication
3. Implement data persistence and retry logic for network outages
4. Add OTA update capability for remote firmware updates