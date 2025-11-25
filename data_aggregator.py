#!/usr/bin/env python3
"""
Data Aggregation Service for BoilerStat
Processes raw sensor data into minute-level utilization percentages using APScheduler.
"""

import psycopg2
from psycopg2.extras import RealDictCursor
import os
import logging
from datetime import datetime, timedelta, timezone
from apscheduler.schedulers.blocking import BlockingScheduler
from apscheduler.triggers.cron import CronTrigger
import signal
import sys

# PostgreSQL configuration from environment variables with defaults
POSTGRES_HOST = os.getenv("POSTGRES_HOST", "localhost")
POSTGRES_PORT = os.getenv("POSTGRES_PORT", "5432")
POSTGRES_DB = os.getenv("POSTGRES_DB")
POSTGRES_USER = os.getenv("POSTGRES_USER")
POSTGRES_PASSWORD = os.getenv("POSTGRES_PASSWORD")
RAW_DATA_RETENTION_HOURS = int(os.getenv("RAW_DATA_RETENTION_HOURS", "3"))
LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO")

# Configure logging
logging.basicConfig(
    level=getattr(logging, LOG_LEVEL.upper()),
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('data_aggregator')

class DatabaseManager:
    """Handles database connections and schema creation."""
    
    def __init__(self):
        # PostgreSQL tables are created by init scripts, no need to initialize here
        pass
    
    def get_connection(self):
        """Get PostgreSQL database connection."""
        return psycopg2.connect(
            host=POSTGRES_HOST,
            port=POSTGRES_PORT,
            database=POSTGRES_DB,
            user=POSTGRES_USER,
            password=POSTGRES_PASSWORD
        )

class DataAggregator:
    """Main aggregation service class."""
    
    def __init__(self):
        self.db_manager = DatabaseManager()
        self.scheduler = BlockingScheduler()
        self.setup_scheduler()
        self.setup_signal_handlers()
    
    def setup_scheduler(self):
        """Configure APScheduler jobs."""
        # Run aggregation every minute at :00 seconds
        self.scheduler.add_job(
            func=self.aggregate_minute_data,
            trigger=CronTrigger(second=0),
            id='minute_aggregation',
            name='Minute Data Aggregation'
        )
        
        # Clean up old raw data every hour at :05 minutes
        self.scheduler.add_job(
            func=self.cleanup_raw_data,
            trigger=CronTrigger(minute=5),
            id='raw_data_cleanup',
            name='Raw Data Cleanup'
        )
        
        # Backfill missing aggregated data every 5 minutes at :02, :07, :12, etc.
        self.scheduler.add_job(
            func=self.backfill_missing_aggregations,
            trigger=CronTrigger(minute='2,7,12,17,22,27,32,37,42,47,52,57'),
            id='backfill_aggregation',
            name='Backfill Missing Aggregations'
        )
    
    def setup_signal_handlers(self):
        """Setup graceful shutdown handlers."""
        signal.signal(signal.SIGINT, self.shutdown)
        signal.signal(signal.SIGTERM, self.shutdown)
    
    def shutdown(self, signum, frame):
        """Gracefully shutdown the scheduler."""
        logger.info(f"Received signal {signum}, shutting down...")
        self.scheduler.shutdown()
        sys.exit(0)
    
    def aggregate_minute_data(self):
        """Aggregate raw data for the previous complete minute."""
        try:
            # Calculate the previous complete minute in UTC
            now = datetime.now(timezone.utc)
            previous_minute = now.replace(second=0, microsecond=0) - timedelta(minutes=1)
            minute_start = previous_minute.strftime('%Y-%m-%d %H:%M:00')
            minute_end = (previous_minute + timedelta(minutes=1)).strftime('%Y-%m-%d %H:%M:00')
            
            logger.info(f"Aggregating data for minute: {minute_start}")
            
            conn = self.db_manager.get_connection()
            cursor = conn.cursor()
            
            # Query raw data for this minute with production priority logic
            cursor.execute('''
                SELECT boiler, zone_1, zone_2, zone_3, zone_4, zone_5, zone_6, is_demo
                FROM boiler_readings
                WHERE timestamp >= %s AND timestamp < %s
                ORDER BY timestamp
            ''', (minute_start, minute_end))
            
            rows = cursor.fetchall()
            
            if not rows:
                logger.debug(f"No data found for minute {minute_start}")
                conn.close()
                return
            
            # Apply production data priority logic
            production_rows = [row for row in rows if row[7] == 0]  # is_demo = 0 means production
            demo_rows = [row for row in rows if row[7] == 1]  # is_demo = 1 means demo
            
            if production_rows:
                # Use only production data if available
                filtered_rows = production_rows
                is_demo_utilization = 0
                logger.info(f"Using {len(production_rows)} production samples (ignoring {len(demo_rows)} demo samples)")
            else:
                # Use demo data only if no production data exists
                filtered_rows = demo_rows
                is_demo_utilization = 1
                logger.info(f"Using {len(demo_rows)} demo samples (no production data available)")
            
            # Calculate utilization percentages from filtered data
            sample_count = len(filtered_rows)
            boiler_on_count = sum(1 for row in filtered_rows if row[0] == 1)
            zone_on_counts = [sum(1 for row in filtered_rows if row[i] == 1) for i in range(1, 7)]
            
            boiler_utilization = (boiler_on_count / sample_count) * 100
            zone_utilizations = [(count / sample_count) * 100 for count in zone_on_counts]
            
            # Insert aggregated data (use ON CONFLICT for idempotency)
            cursor.execute('''
                INSERT INTO minute_utilization
                (minute_timestamp, boiler_utilization, zone_1_utilization, zone_2_utilization,
                 zone_3_utilization, zone_4_utilization, zone_5_utilization, zone_6_utilization,
                 sample_count, is_demo)
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                ON CONFLICT (minute_timestamp)
                DO UPDATE SET
                    boiler_utilization = EXCLUDED.boiler_utilization,
                    zone_1_utilization = EXCLUDED.zone_1_utilization,
                    zone_2_utilization = EXCLUDED.zone_2_utilization,
                    zone_3_utilization = EXCLUDED.zone_3_utilization,
                    zone_4_utilization = EXCLUDED.zone_4_utilization,
                    zone_5_utilization = EXCLUDED.zone_5_utilization,
                    zone_6_utilization = EXCLUDED.zone_6_utilization,
                    sample_count = EXCLUDED.sample_count,
                    is_demo = EXCLUDED.is_demo
            ''', (
                minute_start,
                boiler_utilization,
                *zone_utilizations,
                sample_count,
                is_demo_utilization
            ))
            
            conn.commit()
            conn.close()
            
            logger.info(f"Aggregated {sample_count} samples for {minute_start}: "
                       f"Boiler: {boiler_utilization:.1f}%, "
                       f"Zones: {[f'{u:.1f}%' for u in zone_utilizations]}")
            
        except Exception as e:
            logger.error(f"Error aggregating minute data: {e}")
    
    def aggregate_specific_minute(self, minute_timestamp_str):
        """Aggregate raw data for a specific minute."""
        try:
            minute_start = minute_timestamp_str
            minute_end = (datetime.fromisoformat(minute_start) + timedelta(minutes=1)).strftime('%Y-%m-%d %H:%M:%S')
            
            conn = self.db_manager.get_connection()
            cursor = conn.cursor()
            
            # Query raw data for this specific minute with production priority logic
            cursor.execute('''
                SELECT boiler, zone_1, zone_2, zone_3, zone_4, zone_5, zone_6, is_demo
                FROM boiler_readings
                WHERE timestamp >= %s AND timestamp < %s
                ORDER BY timestamp
            ''', (minute_start, minute_end))
            
            rows = cursor.fetchall()
            
            if not rows:
                logger.debug(f"No data found for minute {minute_start}")
                conn.close()
                return False
            
            # Apply production data priority logic
            production_rows = [row for row in rows if row[7] == 0]  # is_demo = 0 means production
            demo_rows = [row for row in rows if row[7] == 1]  # is_demo = 1 means demo
            
            if production_rows:
                # Use only production data if available
                filtered_rows = production_rows
                is_demo_utilization = 0
                logger.info(f"Backfill: Using {len(production_rows)} production samples (ignoring {len(demo_rows)} demo samples)")
            else:
                # Use demo data only if no production data exists
                filtered_rows = demo_rows
                is_demo_utilization = 1
                logger.info(f"Backfill: Using {len(demo_rows)} demo samples (no production data available)")
            
            # Calculate utilization percentages from filtered data
            sample_count = len(filtered_rows)
            boiler_on_count = sum(1 for row in filtered_rows if row[0] == 1)
            zone_on_counts = [sum(1 for row in filtered_rows if row[i] == 1) for i in range(1, 7)]
            
            boiler_utilization = (boiler_on_count / sample_count) * 100
            zone_utilizations = [(count / sample_count) * 100 for count in zone_on_counts]
            
            # Insert aggregated data
            cursor.execute('''
                INSERT INTO minute_utilization
                (minute_timestamp, boiler_utilization, zone_1_utilization, zone_2_utilization,
                 zone_3_utilization, zone_4_utilization, zone_5_utilization, zone_6_utilization,
                 sample_count, is_demo)
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                ON CONFLICT (minute_timestamp)
                DO UPDATE SET
                    boiler_utilization = EXCLUDED.boiler_utilization,
                    zone_1_utilization = EXCLUDED.zone_1_utilization,
                    zone_2_utilization = EXCLUDED.zone_2_utilization,
                    zone_3_utilization = EXCLUDED.zone_3_utilization,
                    zone_4_utilization = EXCLUDED.zone_4_utilization,
                    zone_5_utilization = EXCLUDED.zone_5_utilization,
                    zone_6_utilization = EXCLUDED.zone_6_utilization,
                    sample_count = EXCLUDED.sample_count,
                    is_demo = EXCLUDED.is_demo
            ''', (
                minute_start,
                boiler_utilization,
                *zone_utilizations,
                sample_count,
                is_demo_utilization
            ))
            
            conn.commit()
            conn.close()
            
            logger.info(f"Backfilled {sample_count} samples for {minute_start}: "
                       f"Boiler: {boiler_utilization:.1f}%, "
                       f"Zones: {[f'{u:.1f}%' for u in zone_utilizations]}")
            return True
            
        except Exception as e:
            logger.error(f"Error aggregating specific minute {minute_timestamp_str}: {e}")
            return False
    
    def backfill_missing_aggregations(self):
        """Find minutes with raw data but no aggregated data and process them."""
        try:
            # Look back up to 24 hours for missing aggregations
            lookback_time = datetime.now(timezone.utc) - timedelta(hours=24)
            lookback_str = lookback_time.strftime('%Y-%m-%d %H:%M:00')
            
            conn = self.db_manager.get_connection()
            cursor = conn.cursor()
            
            # Find distinct minutes in raw data that don't have aggregated data
            cursor.execute('''
                SELECT DISTINCT substring(r.timestamp::text, 1, 16) || ':00' as minute_mark
                FROM boiler_readings r
                LEFT JOIN minute_utilization m ON substring(r.timestamp::text, 1, 16) || ':00' = m.minute_timestamp
                WHERE r.timestamp >= %s 
                  AND m.minute_timestamp IS NULL
                ORDER BY minute_mark
            ''', (lookback_str,))
            
            missing_minutes = cursor.fetchall()
            conn.close()
            
            if not missing_minutes:
                logger.debug("No missing aggregations found")
                return
            
            logger.info(f"Found {len(missing_minutes)} missing aggregations, processing...")
            
            processed_count = 0
            for (minute_mark,) in missing_minutes:
                if self.aggregate_specific_minute(minute_mark):
                    processed_count += 1
            
            logger.info(f"Backfill completed: processed {processed_count} of {len(missing_minutes)} minutes")
            
        except Exception as e:
            logger.error(f"Error during backfill: {e}")
    
    def cleanup_raw_data(self):
        """Remove raw data older than retention period."""
        try:
            cutoff_time = datetime.now(timezone.utc) - timedelta(hours=RAW_DATA_RETENTION_HOURS)
            cutoff_str = cutoff_time.strftime('%Y-%m-%d %H:%M:%S')
            
            conn = self.db_manager.get_connection()
            cursor = conn.cursor()
            
            # Count records to be deleted
            cursor.execute('''
                SELECT COUNT(*) FROM boiler_readings WHERE timestamp < %s
            ''', (cutoff_str,))
            count_to_delete = cursor.fetchone()[0]
            
            if count_to_delete > 0:
                # Delete old records
                cursor.execute('''
                    DELETE FROM boiler_readings WHERE timestamp < %s
                ''', (cutoff_str,))
                
                conn.commit()
                logger.info(f"Deleted {count_to_delete} raw records older than {cutoff_str}")
            else:
                logger.debug("No old raw data to cleanup")
            
            conn.close()
            
        except Exception as e:
            logger.error(f"Error cleaning up raw data: {e}")
    
    def run(self):
        """Start the aggregation service."""
        logger.info("Starting Data Aggregation Service")
        logger.info(f"Raw data retention: {RAW_DATA_RETENTION_HOURS} hours")
        logger.info("Scheduled jobs:")
        for job in self.scheduler.get_jobs():
            logger.info(f"  - {job.name}: {job.trigger}")
        
        try:
            self.scheduler.start()
        except KeyboardInterrupt:
            logger.info("Service stopped by user")
        except Exception as e:
            logger.error(f"Service error: {e}")

def main():
    """Main function to start the aggregation service."""
    
    # Verify database connection
    try:
        db_manager = DatabaseManager()
        conn = db_manager.get_connection()
        conn.close()
        logger.info(f"Connected to PostgreSQL database: {POSTGRES_DB}@{POSTGRES_HOST}:{POSTGRES_PORT}")
    except psycopg2.Error as e:
        logger.error(f"Database connection error: {e}")
        logger.error("Please ensure PostgreSQL is running and credentials are correct.")
        sys.exit(1)
    
    # Start the aggregation service
    aggregator = DataAggregator()
    aggregator.run()

if __name__ == "__main__":
    main()