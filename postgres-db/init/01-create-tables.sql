-- BoilerStat PostgreSQL Database Schema
-- This script creates the main tables for storing raw ESP32 readings and aggregated utilization data

-- Create the main raw readings table
CREATE TABLE IF NOT EXISTS boiler_readings (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP NOT NULL,
    boiler INTEGER NOT NULL CHECK (boiler IN (0, 1)),
    zone_1 INTEGER NOT NULL CHECK (zone_1 IN (0, 1)),
    zone_2 INTEGER NOT NULL CHECK (zone_2 IN (0, 1)),
    zone_3 INTEGER NOT NULL CHECK (zone_3 IN (0, 1)),
    zone_4 INTEGER NOT NULL CHECK (zone_4 IN (0, 1)),
    zone_5 INTEGER NOT NULL CHECK (zone_5 IN (0, 1)),
    zone_6 INTEGER NOT NULL CHECK (zone_6 IN (0, 1)),
    is_demo INTEGER DEFAULT 0 CHECK (is_demo IN (0, 1)),
    received_at TIMESTAMP DEFAULT NOW()
);

-- Add comments for documentation
COMMENT ON TABLE boiler_readings IS 'Raw sensor readings from ESP32 device';
COMMENT ON COLUMN boiler_readings.timestamp IS 'UTC timestamp from ESP32 device';
COMMENT ON COLUMN boiler_readings.boiler IS 'Boiler/burner state: 0=off, 1=on';
COMMENT ON COLUMN boiler_readings.zone_1 IS 'Zone 1 heating call state: 0=off, 1=on';
COMMENT ON COLUMN boiler_readings.zone_2 IS 'Zone 2 heating call state: 0=off, 1=on';
COMMENT ON COLUMN boiler_readings.zone_3 IS 'Zone 3 heating call state: 0=off, 1=on';
COMMENT ON COLUMN boiler_readings.zone_4 IS 'Zone 4 heating call state: 0=off, 1=on';
COMMENT ON COLUMN boiler_readings.zone_5 IS 'Zone 5 heating call state: 0=off, 1=on';
COMMENT ON COLUMN boiler_readings.zone_6 IS 'Zone 6 heating call state: 0=off, 1=on';
COMMENT ON COLUMN boiler_readings.is_demo IS 'Data source: 0=production/real data, 1=demo/mock data';
COMMENT ON COLUMN boiler_readings.received_at IS 'UTC timestamp when data was stored in database';

-- Create the minute-level aggregated utilization table
CREATE TABLE IF NOT EXISTS minute_utilization (
    id SERIAL PRIMARY KEY,
    minute_timestamp TIMESTAMP NOT NULL UNIQUE,
    boiler_utilization DECIMAL(5,2) NOT NULL CHECK (boiler_utilization >= 0 AND boiler_utilization <= 100),
    zone_1_utilization DECIMAL(5,2) NOT NULL CHECK (zone_1_utilization >= 0 AND zone_1_utilization <= 100),
    zone_2_utilization DECIMAL(5,2) NOT NULL CHECK (zone_2_utilization >= 0 AND zone_2_utilization <= 100),
    zone_3_utilization DECIMAL(5,2) NOT NULL CHECK (zone_3_utilization >= 0 AND zone_3_utilization <= 100),
    zone_4_utilization DECIMAL(5,2) NOT NULL CHECK (zone_4_utilization >= 0 AND zone_4_utilization <= 100),
    zone_5_utilization DECIMAL(5,2) NOT NULL CHECK (zone_5_utilization >= 0 AND zone_5_utilization <= 100),
    zone_6_utilization DECIMAL(5,2) NOT NULL CHECK (zone_6_utilization >= 0 AND zone_6_utilization <= 100),
    sample_count INTEGER NOT NULL CHECK (sample_count > 0),
    is_demo INTEGER DEFAULT 0 CHECK (is_demo IN (0, 1)),
    created_at TIMESTAMP DEFAULT NOW()
);

-- Add comments for aggregation table
COMMENT ON TABLE minute_utilization IS 'Minute-level aggregated utilization statistics';
COMMENT ON COLUMN minute_utilization.minute_timestamp IS 'Start of the minute boundary for aggregation (UTC)';
COMMENT ON COLUMN minute_utilization.boiler_utilization IS 'Boiler utilization percentage for this minute (0-100)';
COMMENT ON COLUMN minute_utilization.zone_1_utilization IS 'Zone 1 utilization percentage for this minute (0-100)';
COMMENT ON COLUMN minute_utilization.zone_2_utilization IS 'Zone 2 utilization percentage for this minute (0-100)';
COMMENT ON COLUMN minute_utilization.zone_3_utilization IS 'Zone 3 utilization percentage for this minute (0-100)';
COMMENT ON COLUMN minute_utilization.zone_4_utilization IS 'Zone 4 utilization percentage for this minute (0-100)';
COMMENT ON COLUMN minute_utilization.zone_5_utilization IS 'Zone 5 utilization percentage for this minute (0-100)';
COMMENT ON COLUMN minute_utilization.zone_6_utilization IS 'Zone 6 utilization percentage for this minute (0-100)';
COMMENT ON COLUMN minute_utilization.sample_count IS 'Number of raw readings included in this aggregation';
COMMENT ON COLUMN minute_utilization.is_demo IS 'Data source: 0=production data, 1=demo data';
COMMENT ON COLUMN minute_utilization.created_at IS 'UTC timestamp when aggregation was created';

-- Print confirmation message
SELECT 'BoilerStat tables created successfully!' AS status;