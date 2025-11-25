#!/bin/sh
set -e

echo "Starting BoilerStat Listener..."

# For PostgreSQL, we don't need to initialize - tables are created by init scripts
echo "Using PostgreSQL database: $POSTGRES_DB@$POSTGRES_HOST:$POSTGRES_PORT"

# Start the listener
echo "Starting MQTT listener..."
exec python3 -u /app/mqtt_database_logger.py
