#!/usr/bin/env python3
"""
Database Monitor - Watches for new data in the database and displays it.
"""

import sqlite3
import time
from datetime import datetime

DB_FILE = "boilerstat.db"
CHECK_INTERVAL = 2  # seconds


def get_last_id():
    """Get the highest ID from the database."""
    try:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        cursor.execute('SELECT MAX(id) FROM boiler_readings')
        result = cursor.fetchone()
        conn.close()
        return result[0] if result[0] is not None else 0
    except sqlite3.Error as e:
        print(f"Database error: {e}")
        return 0


def get_new_readings(last_id):
    """Get all readings with ID greater than last_id."""
    try:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        cursor.execute('''
            SELECT id, timestamp, boiler_state, zone_1, zone_2, zone_3,
                   zone_4, zone_5, zone_6, received_at
            FROM boiler_readings
            WHERE id > ?
            ORDER BY id ASC
        ''', (last_id,))
        results = cursor.fetchall()
        conn.close()
        return results
    except sqlite3.Error as e:
        print(f"Database error: {e}")
        return []


def format_reading(reading):
    """Format a reading for display."""
    (id, timestamp, boiler, z1, z2, z3, z4, z5, z6, received_at) = reading

    output = []
    output.append(f"\n{'='*60}")
    output.append(f"Record ID: {id}")
    output.append(f"ESP32 Timestamp: {timestamp}")
    output.append(f"Received At: {received_at}")
    output.append(f"{'-'*60}")
    output.append(f"Boiler State: {'ON' if boiler else 'OFF'} ({boiler})")
    output.append(f"Zone States:")
    output.append(f"  Zone 1: {'ON' if z1 else 'OFF'} ({z1})")
    output.append(f"  Zone 2: {'ON' if z2 else 'OFF'} ({z2})")
    output.append(f"  Zone 3: {'ON' if z3 else 'OFF'} ({z3})")
    output.append(f"  Zone 4: {'ON' if z4 else 'OFF'} ({z4})")
    output.append(f"  Zone 5: {'ON' if z5 else 'OFF'} ({z5})")
    output.append(f"  Zone 6: {'ON' if z6 else 'OFF'} ({z6})")
    output.append(f"{'='*60}")

    return '\n'.join(output)


def main():
    """Main function to monitor database for new entries."""

    # Verify database exists
    try:
        conn = sqlite3.connect(DB_FILE)
        conn.close()
    except sqlite3.Error as e:
        print(f"Database error: {e}")
        print("Please run init_database.py first to create the database.")
        return

    print("BoilerStat Database Monitor")
    print(f"Checking for new data every {CHECK_INTERVAL} seconds...")
    print("Press Ctrl+C to stop\n")

    # Get initial last ID
    last_id = get_last_id()
    print(f"Starting from record ID: {last_id}")

    try:
        while True:
            new_readings = get_new_readings(last_id)

            if new_readings:
                for reading in new_readings:
                    print(format_reading(reading))
                    last_id = reading[0]  # Update last_id to current record ID

            time.sleep(CHECK_INTERVAL)

    except KeyboardInterrupt:
        print("\n\nStopping monitor...")
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()
