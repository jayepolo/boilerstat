#!/usr/bin/env python3
"""
Initialize the BoilerStat SQLite database with the required schema.
"""

import sqlite3
import os

# Configuration from environment variable with default
DB_FILE = os.getenv("DB_FILE", "boilerstat.db")


def init_database():
    """Create the database and tables if they don't exist."""

    # Connect to database (creates file if it doesn't exist)
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()

    # Create readings table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS boiler_readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            boiler_state INTEGER NOT NULL,
            zone_1 INTEGER NOT NULL,
            zone_2 INTEGER NOT NULL,
            zone_3 INTEGER NOT NULL,
            zone_4 INTEGER NOT NULL,
            zone_5 INTEGER NOT NULL,
            zone_6 INTEGER NOT NULL,
            is_demo INTEGER DEFAULT 0,
            received_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    ''')

    # Add is_demo column to existing tables (migration)
    try:
        cursor.execute('ALTER TABLE boiler_readings ADD COLUMN is_demo INTEGER DEFAULT 0')
        print("Added is_demo column to existing boiler_readings table")
    except sqlite3.OperationalError:
        # Column already exists
        pass

    # Create index on timestamp for faster queries
    cursor.execute('''
        CREATE INDEX IF NOT EXISTS idx_timestamp
        ON boiler_readings(timestamp)
    ''')

    conn.commit()
    conn.close()

    print(f"Database '{DB_FILE}' initialized successfully.")
    print("Table 'boiler_readings' created with columns:")
    print("  - id (auto-increment)")
    print("  - timestamp (from ESP32)")
    print("  - boiler_state (0/1)")
    print("  - zone_1 through zone_6 (0/1)")
    print("  - is_demo (0=production, 1=demo)")
    print("  - received_at (auto-generated)")


if __name__ == "__main__":
    init_database()
