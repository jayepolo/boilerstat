#!/usr/bin/env python3
"""
Initialize aggregation database schema for BoilerStat.
Run this to create the minute_utilization table.
"""

import sqlite3
import os
import sys

DB_FILE = os.getenv("DB_FILE", "data/boilerstat.db")

def init_aggregation_schema():
    """Create aggregation tables in the database."""
    
    if not os.path.exists(DB_FILE):
        print(f"Error: Database file not found: {DB_FILE}")
        print("Please run init_database.py first to create the main database.")
        return False
    
    try:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        
        print("Creating minute_utilization table...")
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS minute_utilization (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                minute_timestamp TEXT NOT NULL UNIQUE,
                boiler_utilization REAL NOT NULL,
                zone_1_utilization REAL NOT NULL,
                zone_2_utilization REAL NOT NULL,
                zone_3_utilization REAL NOT NULL,
                zone_4_utilization REAL NOT NULL,
                zone_5_utilization REAL NOT NULL,
                zone_6_utilization REAL NOT NULL,
                sample_count INTEGER NOT NULL,
                is_demo INTEGER DEFAULT 0,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
        ''')

        # Add is_demo column to existing tables (migration)
        try:
            cursor.execute('ALTER TABLE minute_utilization ADD COLUMN is_demo INTEGER DEFAULT 0')
            print("Added is_demo column to existing minute_utilization table")
        except sqlite3.OperationalError:
            # Column already exists
            pass
        
        print("Creating index on minute_timestamp...")
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_minute_timestamp
            ON minute_utilization(minute_timestamp)
        ''')
        
        conn.commit()
        conn.close()
        
        print("✅ Aggregation database schema initialized successfully!")
        return True
        
    except sqlite3.Error as e:
        print(f"❌ Database error: {e}")
        return False
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        return False

if __name__ == "__main__":
    success = init_aggregation_schema()
    sys.exit(0 if success else 1)