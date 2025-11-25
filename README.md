# BoilerStat

IoT project for monitoring HVAC boiler and zone states using ESP32, MQTT, and SQLite.

## Project Overview

- ESP32 device captures boiler state (binary) and 6 zone states (binary)
- Data published to Mosquitto MQTT broker
- Python listener subscribes to MQTT and stores data in SQLite
- Data aggregation service processes minute-level utilization statistics
- Home Assistant integration (future)

## Current Setup

- **MQTT Broker**: 192.168.1.245:1883 (no authentication)
- **Database**: SQLite (boilerstat.db)
- **Topic**: boilerstat/reading
- **ESP32 Hardware**: Production device (MAC: 30:c6:f7:00:43:1c)

## System Status

### ✅ Production Ready Components
- **ESP32 Hardware**: Deployed and operational with enhanced NTP synchronization
- **MQTT Database Logger**: Running in Docker container with UTC timestamp handling
- **Data Aggregator**: Processing raw data into minute-level utilization statistics
- **Database**: Over 30,000 raw readings collected, timezone issues resolved

### 🔧 Recent Updates (November 23, 2025)
- **Enhanced NTP Synchronization**: ESP32 now uses multiple NTP servers with hourly resyncing
- **UTC Timestamp Fix**: Resolved timezone conversion issues in data pipeline
- **Time Accuracy**: ESP32 maintains accurate time with smooth adjustment and drift prevention
- **Database Cleanup**: Removed duplicate records caused by timezone inconsistencies

## Installation

1. Install dependencies:
```bash
pip install -r requirements.txt
```

Or use the provided setup script:
```bash
bash setup_boilerstat.sh
```

2. Initialize the database:
```bash
python3 init_database.py
```

## Testing Scripts

### 1. ESP32 Simulator (`mqtt_simulator.py`)
Emulates an ESP32 device publishing test data to the MQTT broker.

```bash
python3 mqtt_simulator.py
```

Publishes random boiler and zone states every 5 seconds to topic `boilerstat/reading`.

### 2. MQTT Database Logger (`mqtt_database_logger.py`)
Subscribes to MQTT broker and stores incoming data in SQLite database.

```bash
python3 mqtt_database_logger.py
```

### 3. Database Monitor (`verify_data.py`)
Monitors the database for new entries and displays them in the terminal.

```bash
python3 verify_data.py
```

## Usage Workflow

To test the complete data flow, run these three scripts in separate terminal windows:

**Terminal 1** - Start the database logger:
```bash
python3 mqtt_database_logger.py
```

**Terminal 2** - Start the database monitor:
```bash
python3 verify_data.py
```

**Terminal 3** - Start the ESP32 simulator:
```bash
python3 mqtt_simulator.py
```

You should see:
- Terminal 3: Publishing messages to MQTT
- Terminal 1: Receiving messages and storing to database
- Terminal 2: Displaying new database entries

## Data Format

JSON payload published to MQTT:
```json
{
  "timestamp": "2025-11-18 19:45:30",
  "boiler_state": 1,
  "zone_1": 0,
  "zone_2": 1,
  "zone_3": 0,
  "zone_4": 1,
  "zone_5": 0,
  "zone_6": 1
}
```

## Database Schema

### Raw Data Table: `boiler_readings`
- id: Auto-increment primary key
- timestamp: UTC timestamp from ESP32
- boiler_state: 0 or 1
- zone_1 through zone_6: 0 or 1 each
- received_at: Auto-generated UTC timestamp when data stored

### Aggregated Data Table: `minute_utilization`
- id: Auto-increment primary key
- minute_timestamp: Minute boundary UTC timestamp
- boiler_utilization: Percentage utilization (0-100)
- zone_1_utilization through zone_6_utilization: Percentage utilization (0-100)
- sample_count: Number of raw samples in minute
- created_at: Auto-generated timestamp

## Architecture

### Production Deployment (Docker)
```yaml
services:
  boilerstat-listener:     # MQTT subscriber and database logger
  boilerstat-aggregator:   # Data aggregation service
```

### ESP32 Firmware
- **Location**: `/home/jayepolo/esp/esp-idf-project/boilerstat_production/`
- **Features**: Enhanced NTP synchronization, UTC timestamps, GPIO debouncing
- **Build**: ESP-IDF via Docker container
- **Hardware**: Lolin32 board with CP210x UART bridge

## Files

- `init_database.py` - Database initialization script
- `mqtt_database_logger.py` - MQTT subscriber and database logger (corrected UTC handling)
- `data_aggregator.py` - Minute-level data aggregation service
- `mqtt_simulator.py` - ESP32 emulator for testing (deprecated)
- `verify_data.py` - Database monitor script
- `esp32_boilerstat_production.c` - Production ESP32 firmware
- `docker-compose.yml` - Production deployment configuration
- `boilerstat.db` - SQLite database (created after init)
- `requirements.txt` - Python dependencies
- `setup_boilerstat.sh` - Environment setup script

### Current Data Statistics
- **Raw readings**: 30,970+ records and growing
- **Publishing frequency**: Every 5 seconds from ESP32
- **Data retention**: Raw data kept for 3 hours, aggregated data permanent
- **System uptime**: Continuous operation since ESP32 deployment
