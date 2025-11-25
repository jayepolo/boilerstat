#!/usr/bin/env python3
"""
BoilerStat Mode Control Script
Sends MQTT control messages to switch ESP32 between demo and production modes.
"""

import paho.mqtt.client as mqtt
import json
import sys
import time
import argparse
import os

# MQTT Configuration
MQTT_BROKER = os.getenv("MQTT_BROKER", "192.168.1.245")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_CONTROL_TOPIC = os.getenv("MQTT_CONTROL_TOPIC", "boilerstat/control")
MQTT_DATA_TOPIC = os.getenv("MQTT_DATA_TOPIC", "boilerstat/reading")

class ModeController:
    def __init__(self):
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.connected = False
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            print(f"✅ Connected to MQTT broker at {MQTT_BROKER}")
            # Subscribe to data topic to monitor mode changes
            client.subscribe(MQTT_DATA_TOPIC)
        else:
            print(f"❌ Failed to connect to MQTT broker. Return code: {rc}")
            
    def on_disconnect(self, client, userdata, rc):
        self.connected = False
        print(f"🔌 Disconnected from MQTT broker")
        
    def on_message(self, client, userdata, msg):
        """Monitor incoming data messages to see mode changes"""
        if msg.topic == MQTT_DATA_TOPIC:
            try:
                data = json.loads(msg.payload.decode())
                # Print a simple status line showing current data
                timestamp = data.get('timestamp', 'Unknown')
                burner = data.get('burner', 0)
                zones = [data.get(f'zone_{i}', 0) for i in range(1, 7)]
                zone_str = ' '.join(map(str, zones))
                print(f"📊 [{timestamp}] Burner:{burner} Zones:{zone_str}")
            except:
                pass  # Ignore parsing errors
    
    def connect(self):
        """Connect to MQTT broker"""
        try:
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_start()
            
            # Wait for connection
            timeout = 10
            while not self.connected and timeout > 0:
                time.sleep(0.1)
                timeout -= 0.1
                
            return self.connected
        except Exception as e:
            print(f"❌ Connection error: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from MQTT broker"""
        self.client.loop_stop()
        self.client.disconnect()
    
    def set_demo_mode(self, enable_demo=True):
        """Send control message to enable/disable demo mode"""
        if not self.connected:
            print("❌ Not connected to MQTT broker")
            return False
            
        control_message = {
            "demo_mode": enable_demo,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }
        
        message_json = json.dumps(control_message)
        
        try:
            result = self.client.publish(MQTT_CONTROL_TOPIC, message_json)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                mode_text = "DEMO" if enable_demo else "PRODUCTION" 
                print(f"✅ Mode switch command sent: {mode_text}")
                print(f"📤 Message: {message_json}")
                return True
            else:
                print(f"❌ Failed to publish message. Error code: {result.rc}")
                return False
                
        except Exception as e:
            print(f"❌ Error publishing message: {e}")
            return False
    
    def monitor_mode(self, duration=30):
        """Monitor ESP32 messages for specified duration"""
        print(f"🔍 Monitoring ESP32 messages for {duration} seconds...")
        print("   Watch for mode indicators in log messages")
        print("   Press Ctrl+C to stop monitoring early")
        
        try:
            time.sleep(duration)
        except KeyboardInterrupt:
            print("\n⏹️  Monitoring stopped by user")


def main():
    parser = argparse.ArgumentParser(description="Control BoilerStat ESP32 operating mode")
    parser.add_argument("command", choices=["demo", "production", "monitor"], 
                       help="Command: 'demo' for demo mode, 'production' for production mode, 'monitor' to watch messages")
    parser.add_argument("--duration", type=int, default=30,
                       help="Duration for monitoring in seconds (default: 30)")
    
    args = parser.parse_args()
    
    controller = ModeController()
    
    # Connect to MQTT broker
    print(f"🔌 Connecting to MQTT broker at {MQTT_BROKER}...")
    if not controller.connect():
        print("❌ Failed to connect to MQTT broker. Exiting.")
        sys.exit(1)
    
    try:
        if args.command == "demo":
            print("🎭 Switching to DEMO mode...")
            if controller.set_demo_mode(True):
                print("✅ Demo mode command sent successfully")
                print("   ESP32 will start generating mock data")
                print("   LED should flash 5 times quickly to confirm")
            else:
                print("❌ Failed to send demo mode command")
                
        elif args.command == "production":
            print("🏭 Switching to PRODUCTION mode...")
            if controller.set_demo_mode(False):
                print("✅ Production mode command sent successfully") 
                print("   ESP32 will read actual GPIO pins")
                print("   LED should flash 2 times slowly to confirm")
            else:
                print("❌ Failed to send production mode command")
                
        elif args.command == "monitor":
            controller.monitor_mode(args.duration)
            
    except KeyboardInterrupt:
        print("\n⏹️  Operation cancelled by user")
    
    finally:
        controller.disconnect()
        print("🔌 Disconnected from MQTT broker")


if __name__ == "__main__":
    print("🏠 BoilerStat Mode Controller")
    print("=" * 40)
    main()