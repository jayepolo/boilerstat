#!/usr/bin/env python3
"""
ESP32 Simulator - Publishes test boiler and zone data to MQTT broker.
"""

import json
import time
import random
import paho.mqtt.client as mqtt
from datetime import datetime
import os

# Configuration
MQTT_BROKER = os.getenv("MQTT_BROKER", "192.168.1.245")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "boilerstat/reading")
PUBLISH_INTERVAL = 5  # seconds


def generate_reading():
    """Generate a boiler reading with targeted utilization rates."""
    # Target utilization rates: Zone N = N * 10% (Zone 1=10%, Zone 2=20%, etc.)
    # Burner target: 50% utilization
    zone_target_utilization = [0.10, 0.20, 0.30, 0.40, 0.50, 0.60]
    burner_target_utilization = 0.50
    
    # Generate zone states based on target utilization with ±20% variation
    zones = []
    for i in range(6):
        target_rate = zone_target_utilization[i]
        
        # Add ±20% variation (of the target rate itself)
        variation_range = 0.20
        variation = (random.random() - 0.5) * 2.0 * (target_rate * variation_range)
        adjusted_rate = target_rate + variation
        
        # Ensure rate stays within bounds
        adjusted_rate = max(0.0, min(1.0, adjusted_rate))
        
        # Generate zone state based on adjusted rate
        zones.append(1 if random.random() < adjusted_rate else 0)
    
    # Generate burner state with 50% target ±30% variation
    burner_variation_range = 0.30
    burner_variation = (random.random() - 0.5) * 2.0 * (burner_target_utilization * burner_variation_range)
    adjusted_burner_rate = burner_target_utilization + burner_variation
    adjusted_burner_rate = max(0.0, min(1.0, adjusted_burner_rate))
    
    burner = 1 if random.random() < adjusted_burner_rate else 0
    
    # Light correlation - if many zones calling, burner slightly more likely on
    active_zones = sum(zones)
    if active_zones >= 3 and burner == 0 and random.randint(1, 100) <= 30:
        burner = 1
    
    reading = {
        "timestamp": datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        "boiler_state": burner,
        "zone_1": zones[0],
        "zone_2": zones[1],
        "zone_3": zones[2],
        "zone_4": zones[3],
        "zone_5": zones[4],
        "zone_6": zones[5]
    }
    return reading


def on_connect(client, userdata, flags, rc):
    """Callback for when the client connects to the broker."""
    if rc == 0:
        print(f"Connected to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}")
    else:
        print(f"Connection failed with code {rc}")


def on_publish(client, userdata, mid):
    """Callback for when a message is published."""
    pass


def main():
    """Main function to simulate ESP32 publishing data."""

    # Create MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_publish = on_publish

    try:
        # Connect to broker
        print(f"Connecting to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()

        print(f"Publishing to topic: {MQTT_TOPIC}")
        print(f"Interval: {PUBLISH_INTERVAL} seconds")
        print("Press Ctrl+C to stop\n")

        # Publish loop
        count = 1
        while True:
            reading = generate_reading()
            payload = json.dumps(reading)

            result = client.publish(MQTT_TOPIC, payload)

            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"[{count}] Published at {reading['timestamp']}")
                print(f"    Boiler: {reading['boiler_state']} | Zones: "
                      f"{reading['zone_1']} {reading['zone_2']} {reading['zone_3']} "
                      f"{reading['zone_4']} {reading['zone_5']} {reading['zone_6']}")
            else:
                print(f"[{count}] Publish failed with code {result.rc}")

            count += 1
            time.sleep(PUBLISH_INTERVAL)

    except KeyboardInterrupt:
        print("\n\nStopping simulator...")
        client.loop_stop()
        client.disconnect()
        print("Disconnected from broker.")
    except Exception as e:
        print(f"Error: {e}")
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
