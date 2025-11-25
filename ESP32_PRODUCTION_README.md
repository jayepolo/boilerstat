# ESP32 BoilerStat Production Code

This is the production ESP32 code for reading real boiler and zone control signals and publishing them via MQTT to your BoilerStat system.

## Hardware Setup - Lolin32 ESP32 Board

### GPIO Pin Assignments

| Signal | GPIO Pin | Description |
|--------|----------|-------------|
| Boiler State | GPIO 12 | Boiler on/off signal input |
| Zone 1 | GPIO 13 | Zone 1 call input |
| Zone 2 | GPIO 14 | Zone 2 call input |
| Zone 3 | GPIO 27 | Zone 3 call input |
| Zone 4 | GPIO 26 | Zone 4 call input |
| Zone 5 | GPIO 25 | Zone 5 call input |
| Zone 6 | GPIO 33 | Zone 6 call input |
| Status LED | GPIO 2 | Built-in LED (status indicator) |

### Signal Interface

**Input Voltage Levels:**
- **HIGH (3.3V)** = Zone/Boiler ON (24V signal present)
- **LOW (0V)** = Zone/Boiler OFF (no signal)

**Signal Conditioning Required:**
You'll need a voltage divider or optocoupler circuit to safely convert 24V heating system signals to 3.3V ESP32 levels.

**Example Voltage Divider Circuit (per input):**
```
24V Signal ----[10kΩ]----+----[3.3kΩ]---- GND
                         |
                      GPIO Pin
```

This gives approximately 6V max input to GPIO (still safe for ESP32 which tolerates up to 3.6V).

**Better Option - Optocoupler Isolation:**
- Use PC817 optocouplers for full electrical isolation
- Protects ESP32 from heating system electrical noise
- Safer installation

## Key Features

### Input Debouncing
- 100ms debounce time to filter electrical noise
- Requires 3 consecutive stable readings before state change
- Prevents false triggering from relay contact bounce

### LED Status Indicators
- **WiFi Connected**: LED flashes 10 times rapidly
- **MQTT Publish**: LED flashes 3 times for each data transmission
- **Continuous Operation**: LED off during normal operation

### Network Resilience
- Automatic WiFi reconnection (up to 10 attempts)
- MQTT client auto-reconnect on connection loss
- Network status logging for troubleshooting

### Time Synchronization
- Uses NTP to synchronize with internet time servers
- Timestamps in local time (EST) for consistency with simulator
- Automatic timezone handling

## Installation Steps

### 1. Copy to ESP-IDF Project
```bash
# In ESP-IDF container
cp /path/to/esp32_boilerstat_production.c /project/boilerstat_production/main/main.c
```

### 2. Build the Project
```bash
# In ESP-IDF container
cd /project/boilerstat_production
idf.py build
```

### 3. Flash to ESP32
```bash
# Connect ESP32 via USB and flash
idf.py flash
```

### 4. Monitor Serial Output
```bash
idf.py monitor
```

## Configuration

Update these constants in the code before building:

```c
#define WIFI_SSID "Your-WiFi-Network"
#define WIFI_PASSWORD "Your-WiFi-Password"  
#define MQTT_BROKER_URI "mqtt://192.168.1.245:1883"
```

## Expected Serial Output

```
ESP32 BoilerStat Production Starting...
========================================
LED initialized on GPIO 2
GPIO pins configured:
  Boiler State: GPIO 12
  Zone 1: GPIO 13
  Zone 2: GPIO 14
  ...
Connected to WiFi network: Your-WiFi-Network
Got IP: 192.168.1.123
MQTT_EVENT_CONNECTED
Time synchronized
GPIO reading task started
Sensor data publishing task started
[1] Published at 2025-11-22 20:30:00
    Boiler: 1 | Zones: 1 0 0 1 0 0
    JSON: {"timestamp":"2025-11-22 20:30:00","boiler_state":1,"zone_1":1,...}
```

## Data Format

The ESP32 publishes JSON messages to MQTT topic `boilerstat/reading`:

```json
{
  "timestamp": "2025-11-22 20:30:00",
  "boiler_state": 1,
  "zone_1": 1,
  "zone_2": 0,
  "zone_3": 0,
  "zone_4": 1,
  "zone_5": 0,
  "zone_6": 0
}
```

This format is compatible with your existing MQTT database logger.

## Troubleshooting

### WiFi Connection Issues
- Verify SSID and password in code
- Check signal strength at installation location
- Monitor serial output for connection attempts

### GPIO Reading Issues
- Verify input voltage levels with multimeter
- Check wiring connections to GPIO pins
- Monitor state change logs in serial output
- Test with manual 3.3V signals before connecting heating system

### MQTT Publishing Issues
- Verify MQTT broker IP address and port
- Check network connectivity from ESP32
- Ensure broker allows connections from ESP32's IP

### Power Supply
- ESP32 requires stable 5V USB power or 3.3V regulated supply
- Insufficient power can cause WiFi connection problems
- Consider using external power adapter for reliability

## Safety Notes

⚠️ **ELECTRICAL SAFETY WARNING** ⚠️
- Heating systems operate at 24V AC which can be dangerous
- Use proper voltage isolation (optocouplers recommended)
- Turn off heating system power during installation
- Consider professional installation if unsure about electrical connections
- Test thoroughly before leaving unattended

## Integration with BoilerStat System

Once running, this ESP32 will:

1. **Replace the simulator** - Send real data instead of random values
2. **Integrate seamlessly** - Uses same MQTT topic and JSON format
3. **Provide real insights** - Actual boiler and zone runtime data
4. **Enable analytics** - Real utilization percentages and efficiency metrics
5. **Support delayed data** - Backfill mechanism handles network outages

The production ESP32 works with your existing:
- MQTT broker (Mosquitto)
- Database logger (Python)
- Data aggregator (minute utilization)
- Any future Home Assistant integration