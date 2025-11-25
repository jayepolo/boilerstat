# Docker Deployment Guide

## Architecture Overview

```
┌─────────────────────────────┐
│   Mosquitto Container       │
│   192.168.1.245:1883        │
└─────────────────────────────┘
            ↓ MQTT
┌─────────────────────────────┐
│  BoilerStat Listener        │
│  - Python MQTT Client       │
│  - SQLite Database          │
└─────────────────────────────┘
            ↓
┌─────────────────────────────┐
│  Docker Volume              │
│  boilerstat-data            │
│  /data/boilerstat.db        │
└─────────────────────────────┘
```

## Why One Container?

**SQLite is file-based, not a server process**
- Unlike PostgreSQL/MySQL, SQLite doesn't run as a daemon
- It's a library that reads/writes a database file
- No benefit to running it in a separate container

**Benefits of this architecture:**
- Simple deployment and management
- Minimal resource usage
- Data persists in Docker volume
- Easy to backup (just backup the volume)

## Deployment Steps

### 1. Transfer Files to Ubuntu Host

From your local machine, copy the project to your Ubuntu host:

```bash
# Option A: Using scp
scp -r /Users/jpolo/Documents/GitHub/boilerstat user@192.168.1.245:~/

# Option B: Using rsync (excludes unnecessary files)
rsync -av --exclude 'boilerstat_env' --exclude '*.db' \
  /Users/jpolo/Documents/GitHub/boilerstat user@192.168.1.245:~/
```

### 2. SSH to Ubuntu Host

```bash
ssh user@192.168.1.245
cd ~/boilerstat
```

### 3. Configure Networking (IMPORTANT)

You need to determine how your Mosquitto container is networked:

#### Option A: Mosquitto on Host Network

If Mosquitto is using host network mode:

Edit `docker-compose.yml`:
```yaml
services:
  boilerstat-listener:
    # ... other config ...
    network_mode: host
    # Remove the networks section
```

#### Option B: Mosquitto on Custom Network

If Mosquitto is on a custom Docker network:

```bash
# Find the network name
docker network ls

# Connect your listener to the same network
# Edit docker-compose.yml to use that network
```

#### Option C: Mosquitto on Default Bridge (Current Setup)

The current configuration assumes Mosquitto is accessible at 192.168.1.245:1883
from within Docker containers. This works if:
- Mosquitto has port 1883 published to the host
- The listener can reach the host IP

### 4. Build and Start the Container

```bash
# Build the image
docker compose build

# Start the container
docker compose up -d

# View logs
docker compose logs -f
```

### 5. Verify It's Working

```bash
# Check container status
docker compose ps

# Watch logs in real-time
docker compose logs -f boilerstat-listener

# Check if data is being stored
docker compose exec boilerstat-listener \
  sqlite3 /data/boilerstat.db "SELECT COUNT(*) FROM boiler_readings;"
```

## Data Persistence

The database is stored in a Docker volume named `boilerstat-data`.

### View Volume Location

```bash
docker volume inspect boilerstat-data
```

### Backup Database

```bash
# Method 1: Copy from volume to host
docker compose cp boilerstat-listener:/data/boilerstat.db ./backup.db

# Method 2: Using docker run
docker run --rm -v boilerstat-data:/data -v $(pwd):/backup \
  alpine cp /data/boilerstat.db /backup/boilerstat-backup-$(date +%Y%m%d).db
```

### Restore Database

```bash
# Copy backup into volume
docker compose cp ./backup.db boilerstat-listener:/data/boilerstat.db

# Restart container to pick up changes
docker compose restart
```

### Use Specific Host Directory (Optional)

If you want the database in a specific location on the host, edit `docker-compose.yml`:

```yaml
volumes:
  boilerstat-data:
    driver: local
    driver_opts:
      type: none
      o: bind
      device: /home/your-user/boilerstat-data  # Your chosen path
```

Make sure the directory exists first:
```bash
mkdir -p /home/your-user/boilerstat-data
```

## Environment Variables

You can customize the configuration by editing `docker-compose.yml`:

```yaml
environment:
  - MQTT_BROKER=mosquitto          # Use container name if on same network
  - MQTT_PORT=1883
  - MQTT_TOPIC=boilerstat/reading
  - DB_FILE=/data/boilerstat.db
```

## Querying the Database from Host

### Install SQLite on Ubuntu

```bash
sudo apt update && sudo apt install -y sqlite3
```

### Query the Database

```bash
# Method 1: Execute SQL in container
docker compose exec boilerstat-listener \
  sqlite3 /data/boilerstat.db "SELECT * FROM boiler_readings ORDER BY id DESC LIMIT 10;"

# Method 2: Access database file directly (if using bind mount)
sqlite3 /path/to/boilerstat-data/boilerstat.db

# Method 3: Interactive shell
docker compose exec boilerstat-listener sqlite3 /data/boilerstat.db
```

## Container Management

```bash
# Start
docker compose up -d

# Stop
docker compose down

# Stop and remove volume (CAREFUL - deletes data!)
docker compose down -v

# Restart
docker compose restart

# View logs
docker compose logs -f

# Update code and rebuild
docker compose down
docker compose build --no-cache
docker compose up -d

# Shell access
docker compose exec boilerstat-listener /bin/sh
```

## Troubleshooting

### Container Can't Connect to Mosquitto

1. Check Mosquitto container network:
```bash
docker inspect <mosquitto-container-name> | grep NetworkMode
```

2. Test connectivity from listener container:
```bash
docker compose exec boilerstat-listener ping 192.168.1.245
docker compose exec boilerstat-listener nc -zv 192.168.1.245 1883
```

3. If using container names, ensure they're on the same network:
```bash
# List Mosquitto's networks
docker inspect <mosquitto-container> | grep -A 10 Networks

# Add that network to docker-compose.yml
```

### Database Permissions Issues

```bash
# Check volume permissions
docker compose exec boilerstat-listener ls -la /data

# Fix permissions if needed
docker compose exec boilerstat-listener chown -R root:root /data
```

### View All Data

```bash
# Get row count
docker compose exec boilerstat-listener \
  sqlite3 /data/boilerstat.db "SELECT COUNT(*) FROM boiler_readings;"

# View recent entries
docker compose exec boilerstat-listener \
  sqlite3 /data/boilerstat.db \
  "SELECT id, timestamp, boiler_state FROM boilerstat.db ORDER BY id DESC LIMIT 5;"
```

## Monitoring

### Create a systemd service (Optional)

Create `/etc/systemd/system/boilerstat.service`:

```ini
[Unit]
Description=BoilerStat MQTT Logger
Requires=docker.service
After=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=/home/your-user/boilerstat
ExecStart=/usr/bin/docker compose up -d
ExecStop=/usr/bin/docker compose down
TimeoutStartSec=0

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable boilerstat
sudo systemctl start boilerstat
```

## Home Assistant Integration

Once the listener is running and collecting data, you can access the database from Home Assistant using:

1. **SQLite Sensor** - Query the database directly
2. **MQTT Sensors** - Subscribe to the same MQTT topic
3. **RESTful API** - Create a simple API endpoint (future enhancement)

The recommended approach is to use MQTT sensors in Home Assistant, subscribing to the same `boilerstat/reading` topic that the listener uses.
