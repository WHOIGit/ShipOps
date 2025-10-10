#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

ConfigDir=/home/admin_paul.mena/config
ConfigFile1=${ConfigDir}/vaisala_port.yaml
ConfigFile2=${ConfigDir}/vaisala_star.yaml
ConfigFile3=${ConfigDir}/vaisala_fluor.yaml

# Listen for serial input and write to log file and UDP
# Use listener with config file - no other arguments
# add '-v' to crank up the logging level

logger/listener/listen.py --config_file ${ConfigFile1} 

# Deactivate virtual environment

deactivate
