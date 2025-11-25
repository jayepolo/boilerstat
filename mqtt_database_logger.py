#!/usr/bin/env python3
"""
MQTT Listener that subscribes to BoilerStat data and logs it to PostgreSQL database.
"""

import json
import psycopg2
from psycopg2.extras import RealDictCursor
import os
import paho.mqtt.client as mqtt
from datetime import datetime, timezone, timedelta

# Configuration from environment variables with defaults
MQTT_BROKER = os.getenv("MQTT_BROKER", "192.168.1.245")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "boilerstat/reading")

# PostgreSQL configuration
POSTGRES_HOST = os.getenv("POSTGRES_HOST", "localhost")
POSTGRES_PORT = os.getenv("POSTGRES_PORT", "5432")
POSTGRES_DB = os.getenv("POSTGRES_DB")
POSTGRES_USER = os.getenv("POSTGRES_USER")
POSTGRES_PASSWORD = os.getenv("POSTGRES_PASSWORD")


def get_db_connection():
    """Get PostgreSQL database connection."""
    return psycopg2.connect(
        host=POSTGRES_HOST,
        port=POSTGRES_PORT,
        database=POSTGRES_DB,
        user=POSTGRES_USER,
        password=POSTGRES_PASSWORD
    )


def on_connect(client, userdata, flags, rc):
    """Callback for when the client connects to the broker."""
    if rc == 0:
        print(f"Connected to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}")
        client.subscribe(MQTT_TOPIC)
        print(f"Subscribed to topic: {MQTT_TOPIC}")
    else:
        print(f"Connection failed with code {rc}")


def on_message(client, userdata, msg):
    """Callback for when a message is received from the broker."""
    try:
        # Parse JSON payload
        payload = json.loads(msg.payload.decode())

        # Handle timestamp from ESP32 (already in UTC)
        try:
            # Parse the incoming timestamp (ESP32 now sends UTC directly)
            incoming_timestamp = datetime.fromisoformat(payload['timestamp'].replace('Z', ''))
            if incoming_timestamp.tzinfo is None:
                # ESP32 sends UTC timestamps, use them directly
                utc_timestamp = payload['timestamp']
            else:
                utc_timestamp = incoming_timestamp.astimezone(timezone.utc).strftime('%Y-%m-%d %H:%M:%S')
        except (ValueError, AttributeError):
            # Fallback to current UTC time if parsing fails
            utc_timestamp = datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S')

        # Handle both old "boiler_state" and new "burner" field names
        boiler_value = payload.get('burner', payload.get('boiler_state', 0))
        
        # Handle is_demo flag (default to production mode if not present)
        is_demo = payload.get('is_demo', False)
        is_demo_int = 1 if is_demo else 0
        
        print(f"\n[{datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S')} UTC] Received data:")
        print(f"  Original Timestamp: {payload['timestamp']}")
        print(f"  UTC Timestamp: {utc_timestamp}")
        print(f"  Burner: {boiler_value}")
        print(f"  Zones: {payload['zone_1']}, {payload['zone_2']}, {payload['zone_3']}, "
              f"{payload['zone_4']}, {payload['zone_5']}, {payload['zone_6']}")
        print(f"  Mode: {'DEMO' if is_demo else 'PRODUCTION'}")

        # Store in database
        conn = get_db_connection()
        cursor = conn.cursor()

        cursor.execute('''
            INSERT INTO boiler_readings
            (timestamp, boiler, zone_1, zone_2, zone_3, zone_4, zone_5, zone_6, is_demo)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
        ''', (
            utc_timestamp,
            boiler_value,
            payload['zone_1'],
            payload['zone_2'],
            payload['zone_3'],
            payload['zone_4'],
            payload['zone_5'],
            payload['zone_6'],
            is_demo_int
        ))

        conn.commit()
        conn.close()

        print("  -> Data stored in database")

    except json.JSONDecodeError as e:
        print(f"Error decoding JSON: {e}")
    except KeyError as e:
        print(f"Missing key in payload: {e}")
    except sqlite3.Error as e:
        print(f"Database error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")


def on_disconnect(client, userdata, rc):
    """Callback for when the client disconnects from the broker."""
    if rc != 0:
        print(f"Unexpected disconnection (code {rc})")


def main():
    """Main function to start the MQTT listener."""

    # Verify database connection
    try:
        conn = get_db_connection()
        conn.close()
        print(f"Connected to PostgreSQL database: {POSTGRES_DB}@{POSTGRES_HOST}:{POSTGRES_PORT}")
    except psycopg2.Error as e:
        print(f"Database connection error: {e}")
        print("Please ensure PostgreSQL is running and credentials are correct.")
        return

    # Create MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect

    try:
        # Connect to broker
        print(f"Connecting to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)

        # Start listening loop
        print("Starting listener... (Press Ctrl+C to stop)")
        client.loop_forever()

    except KeyboardInterrupt:
        print("\nStopping listener...")
        client.disconnect()
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()
