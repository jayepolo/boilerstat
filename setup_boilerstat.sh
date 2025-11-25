#!/bin/bash
# setup_boilerstat.sh

# Create virtual environment
python3 -m venv boilerstat_env

# Activate environment
source boilerstat_env/bin/activate

# Upgrade pip
pip install --upgrade pip

# Install required packages
pip install \
    paho-mqtt \
    pandas \
    pyyaml \
    sqlite3

# Create requirements file for future reference
pip freeze > requirements.txt

# Provide instructions
echo "Virtual environment created and packages installed."
echo "To activate: source boilerstat_env/bin/activate"
echo "To deactivate: deactivate"
```

#### Project Structure
```
boilerstat/
│
├── venv/                 # Virtual environment
├── setup_boilerstat.sh   # Setup script
├── requirements.txt      # Dependency list
│
├── mqtt_simulator.py
├── mqtt_database_logger.py
├── verify_data.py
└── boilerstat.db
