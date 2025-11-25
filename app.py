#!/usr/bin/env python3
"""
Flask web application for BoilerStat monitoring dashboard.
Provides REST API endpoints for current status and utilization trends.
"""

from flask import Flask, jsonify, send_from_directory, request, Response
from flask_cors import CORS
import psycopg2
from psycopg2.extras import RealDictCursor
import os
import json
import time
import threading
from datetime import datetime, timedelta
import paho.mqtt.client as mqtt

app = Flask(__name__, static_folder='build')
CORS(app)  # Enable CORS for React development

# PostgreSQL configuration
POSTGRES_HOST = os.getenv("POSTGRES_HOST", "localhost")
POSTGRES_PORT = os.getenv("POSTGRES_PORT", "5432")
POSTGRES_DB = os.getenv("POSTGRES_DB")
POSTGRES_USER = os.getenv("POSTGRES_USER")
POSTGRES_PASSWORD = os.getenv("POSTGRES_PASSWORD")
MQTT_BROKER = os.getenv("MQTT_BROKER", "192.168.1.245")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_CONTROL_TOPIC = os.getenv("MQTT_CONTROL_TOPIC", "boilerstat/control")
MQTT_DATA_TOPIC = os.getenv("MQTT_DATA_TOPIC", "boilerstat/reading")

# Global variables for ESP32 mode control
current_mode = "unknown"
mqtt_client = None

def get_db_connection():
    """Get PostgreSQL database connection."""
    conn = psycopg2.connect(
        host=POSTGRES_HOST,
        port=POSTGRES_PORT,
        database=POSTGRES_DB,
        user=POSTGRES_USER,
        password=POSTGRES_PASSWORD,
        cursor_factory=RealDictCursor
    )
    return conn

@app.route('/api/status')
def get_current_status():
    """Get the current status of burner and all zones from latest reading."""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Get the most recent reading
        cursor.execute('''
            SELECT timestamp, boiler as burner, zone_1, zone_2, zone_3, zone_4, zone_5, zone_6
            FROM boiler_readings
            ORDER BY timestamp DESC
            LIMIT 1
        ''')
        
        row = cursor.fetchone()
        conn.close()
        
        if row:
            return jsonify({
                'timestamp': row['timestamp'],
                'burner': row['burner'],
                'zone_1': row['zone_1'],
                'zone_2': row['zone_2'],
                'zone_3': row['zone_3'],
                'zone_4': row['zone_4'],
                'zone_5': row['zone_5'],
                'zone_6': row['zone_6']
            })
        else:
            return jsonify({'error': 'No data available'}), 404
            
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/utilization')
def get_utilization_data():
    """Get utilization trend data for the last 1 hour."""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Get utilization data from the last 1 hour
        cursor.execute('''
            SELECT 
                minute_timestamp,
                boiler_utilization as burner_utilization,
                zone_1_utilization,
                zone_2_utilization,
                zone_3_utilization,
                zone_4_utilization,
                zone_5_utilization,
                zone_6_utilization
            FROM minute_utilization
            WHERE minute_timestamp >= NOW() - INTERVAL '1 hour'
            ORDER BY minute_timestamp ASC
        ''')
        
        rows = cursor.fetchall()
        conn.close()
        
        # Convert to format suitable for charting
        data = []
        for row in rows:
            data.append({
                'timestamp': row['minute_timestamp'],
                'burner': row['burner_utilization'],
                'zone_1': row['zone_1_utilization'],
                'zone_2': row['zone_2_utilization'],
                'zone_3': row['zone_3_utilization'],
                'zone_4': row['zone_4_utilization'],
                'zone_5': row['zone_5_utilization'],
                'zone_6': row['zone_6_utilization']
            })
        
        return jsonify(data)
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/health')
def health_check():
    """Health check endpoint."""
    return jsonify({'status': 'healthy', 'timestamp': datetime.utcnow().isoformat()})

@app.route('/api/mode', methods=['GET'])
def get_mode():
    """Get current ESP32 operating mode."""
    return jsonify({'mode': current_mode})

@app.route('/api/mode', methods=['POST'])
def set_mode():
    """Set ESP32 operating mode via MQTT."""
    try:
        data = request.get_json()
        if not data or 'mode' not in data:
            return jsonify({'error': 'Mode not specified'}), 400
            
        mode = data['mode'].lower()
        if mode not in ['demo', 'production']:
            return jsonify({'error': 'Invalid mode. Use "demo" or "production"'}), 400
            
        # Send MQTT control message
        demo_mode = (mode == 'demo')
        control_message = {
            "demo_mode": demo_mode,
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }
        
        if mqtt_client and mqtt_client.is_connected():
            result = mqtt_client.publish(MQTT_CONTROL_TOPIC, json.dumps(control_message))
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                return jsonify({'success': True, 'mode': mode, 'message': 'Mode change command sent'})
            else:
                return jsonify({'error': 'Failed to send MQTT command'}), 500
        else:
            return jsonify({'error': 'MQTT client not connected'}), 500
            
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# Removed Live ESP32 Stream functionality

def init_mqtt():
    """Initialize MQTT client for mode control and message monitoring."""
    global mqtt_client, current_mode
    
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("✅ Connected to MQTT broker for web interface")
            client.subscribe(MQTT_DATA_TOPIC)
        else:
            print(f"❌ Failed to connect to MQTT broker. Return code: {rc}")
            
    def on_message(client, userdata, msg):
        global current_mode
        if msg.topic == MQTT_DATA_TOPIC:
            try:
                data = json.loads(msg.payload.decode())
                is_demo = data.get('is_demo', False)
                
                # Update current mode based on ESP32 flag
                current_mode = "demo" if is_demo else "production"
                
            except Exception as e:
                print(f"Error processing MQTT message: {e}")
    
    mqtt_client = mqtt.Client()
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        return True
    except Exception as e:
        print(f"Failed to connect to MQTT broker: {e}")
        return False

# Serve React App
@app.route('/')
def serve_react_app():
    return send_from_directory(app.static_folder, 'index.html')

@app.route('/<path:path>')
def static_proxy(path):
    # Serve static files from React build
    return send_from_directory(app.static_folder, path)

if __name__ == '__main__':
    # Check PostgreSQL database connection
    try:
        conn = get_db_connection()
        conn.close()
        print(f"Connected to PostgreSQL database: {POSTGRES_DB}@{POSTGRES_HOST}:{POSTGRES_PORT}")
    except psycopg2.Error as e:
        print(f"Database connection error: {e}")
        print("Please ensure PostgreSQL is running and credentials are correct.")
        exit(1)
    
    print(f"Starting BoilerStat Web Dashboard...")
    print(f"Database: {POSTGRES_DB}@{POSTGRES_HOST}:{POSTGRES_PORT}")
    print(f"API endpoints available at:")
    print(f"  - http://localhost:5000/api/status")
    print(f"  - http://localhost:5000/api/utilization") 
    print(f"  - http://localhost:5000/api/health")
    print(f"  - http://localhost:5000/api/mode (GET/POST)")
    
    # Initialize MQTT connection
    init_mqtt()
    
    app.run(host=os.getenv("FLASK_HOST", "0.0.0.0"), 
            port=int(os.getenv("FLASK_PORT", "5000")), 
            debug=os.getenv("FLASK_DEBUG", "false").lower() == "true", 
            threaded=True)