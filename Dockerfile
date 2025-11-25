FROM python:3.11-slim

# Set working directory
WORKDIR /app

# Install system packages
RUN apt-get update && \
    apt-get install -y --no-install-recommends sqlite3 && \
    rm -rf /var/lib/apt/lists/*

# Install Python dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt


# Copy application files
COPY mqtt_database_logger.py .
COPY init_database.py .
COPY verify_data.py .
COPY data_aggregator.py .
COPY entrypoint.sh .

# Make entrypoint executable
RUN chmod +x entrypoint.sh

# Create data directory for database
RUN mkdir -p /data

# Set environment variables with defaults
ENV MQTT_BROKER=192.168.1.245 \
    MQTT_PORT=1883 \
    MQTT_TOPIC=boilerstat/reading \
    DB_FILE=/data/boilerstat.db

# Use entrypoint script
ENTRYPOINT ["/app/entrypoint.sh"]
