# BoilerStat Setup Summary

## Project Overview
BoilerStat is a Python-based IoT monitoring system for boiler statistics using MQTT messaging and SQLite database logging. The system captures boiler state and zone data from ESP32 devices via MQTT and stores it for analysis.

### Project Files
- **mqtt_database_logger.py** - Main MQTT listener that receives boiler data and logs to database
- **mqtt_simulator.py** - ESP32 data simulator for testing (generates mock boiler readings)
- **init_database.py** - Database initialization script (creates tables)
- **verify_data.py** - Real-time database monitor for viewing incoming data
- **Dockerfile** - Container configuration for the MQTT listener
- **docker-compose.yml** - Multi-container orchestration setup
- **entrypoint.sh** - Container startup script
- **setup_boilerstat.sh** - Automated setup script
- **requirements.txt** - Python dependencies
- **README.md** - Project documentation
- **DOCKER_DEPLOYMENT.md** - Deployment guide

### Container Architecture
- **boilerstat-listener** - Main container running `mqtt_database_logger.py`
  - Connects to MQTT broker (mosquitto) 
  - Listens on topic `boilerstat/reading`
  - Stores data to SQLite database at `/data/boilerstat.db`
  - Database persisted via Docker volume and host mount at `./data/`
- **External dependencies**:
  - MQTT broker (mosquitto) - receives ESP32 sensor data
  - Traefik network - for container networking

### Data Flow
1. ESP32 devices → MQTT broker (mosquitto) → boilerstat-listener container
2. Container processes MQTT messages and stores to SQLite database
3. Database accessible from host at `./data/boilerstat.db` for monitoring

## Work Completed
1. **Added database verification tool to Docker container**
   - Modified Dockerfile to include `verify_data.py`
   - Updated `.dockerignore` to allow `verify_data.py` in build context

2. **Enabled host access to SQLite database**
   - Modified `docker-compose.yml` to mount database to host filesystem
   - Database now accessible at `./data/boilerstat.db` on host

3. **Installed SQLite monitoring tools**
   - Installed pipx package manager
   - Installed sqlite-web for web-based database viewing

## Commands to Recreate Setup

### 1. Build and Run Container
```bash
# Stop existing container
docker compose down

# Build with verify_data.py included
docker compose up -d --build
```

### 2. Access Database from Host
```bash
# View recent data via command line
sqlite3 ./data/boilerstat.db "SELECT * FROM boiler_readings ORDER BY id DESC LIMIT 10;"

# Live monitoring (updates every 2 seconds)
watch -n 2 'sqlite3 ./data/boilerstat.db "SELECT * FROM boiler_readings ORDER BY id DESC LIMIT 10;"'
```

### 3. Install Web-based SQLite Viewer
```bash
# Install pipx if not available
sudo apt install pipx

# Install sqlite-web
pipx install sqlite-web

# Add to PATH
pipx ensurepath
source ~/.bashrc

# Run web interface
source ~/.bashrc  
sqlite_web ./data/boilerstat.db
# Then open http://localhost:8080 in browser
```

### 4. Alternative: Run Verification Script in Container
```bash
# If needed, run verification tool inside container
docker exec -e DB_FILE=/data/boilerstat.db boilerstat-listener python3 verify_data.py
```

## Key Files Modified
- `Dockerfile` - Added `verify_data.py` to container
- `.dockerignore` - Removed `verify_data.py` from ignore list
- `docker-compose.yml` - Added `./data:/data` volume mount

## Database Access
- **Host path**: `./data/boilerstat.db`
- **Container path**: `/data/boilerstat.db`
- **Table**: `boiler_readings`
- **Columns**: `id, timestamp, boiler_state, zone_1, zone_2, zone_3, zone_4, zone_5, zone_6, received_at`

## Monitoring Options
1. **Command line**: Direct sqlite3 queries
2. **Web interface**: sqlite-web at http://localhost:8080
3. **Container exec**: Run verification script inside container
4. **GUI**: DB Browser for SQLite (native apt package, not snap)
