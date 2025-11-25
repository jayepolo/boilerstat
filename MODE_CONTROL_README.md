# BoilerStat Mode Control

The ESP32 now supports two operating modes that can be switched remotely via MQTT:

## Operating Modes

### Production Mode (Default)
- Reads actual GPIO pin states from your HVAC system
- Use when ESP32 is connected to real boiler/zone control wires
- LED flashes 2 times slowly when switching to this mode

### Demo Mode
- Generates realistic mock heating system data
- Useful for testing, demonstrations, or when not connected to HVAC
- Creates realistic patterns based on time of day
- LED flashes 5 times quickly when switching to this mode

## Mode Control Script

Use the `mode_control.py` script to switch modes:

```bash
# Switch to demo mode
python3 mode_control.py demo

# Switch to production mode  
python3 mode_control.py production

# Monitor ESP32 messages
python3 mode_control.py monitor --duration 60
```

## MQTT Control Protocol

The ESP32 subscribes to: `boilerstat/control`

Control message format:
```json
{
    "demo_mode": true,
    "timestamp": "2025-11-23 17:30:00"
}
```

- `demo_mode: true` = Enable demo mode
- `demo_mode: false` = Enable production mode

## Demo Data Characteristics

The demo mode generates realistic heating system behavior:

- **Time-based patterns**: Higher activity during morning (6-8 AM) and evening (5-8 PM) 
- **Seasonal variation**: Adjustable base activity levels
- **Realistic cycling**: Burner cycles every 15-20 seconds when active
- **Zone correlation**: Zones more likely to be active when burner is running
- **Individual zone behavior**: Each zone has different cycling patterns

## Visual Feedback

The ESP32 provides LED feedback when mode changes:
- **Demo mode**: 5 quick flashes (100ms on/off)
- **Production mode**: 2 slow flashes (500ms on/off)

## Implementation Notes

- Mode state persists until explicitly changed
- ESP32 logs current mode with each data publish
- Web dashboard will show generated demo data when in demo mode
- No restart required - mode switches instantly