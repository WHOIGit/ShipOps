#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

ConfigDir=/home/admin_paul.mena/config
ConfigFile=${ConfigDir}/sbe48.yaml

# Listen for input and write to log file and UDP

# Read from serial port reference the YAML file
logger/listener/listen.py --config_file ${ConfigFile}

# Deactivate virtual environment

deactivate
