#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

ConfigDir=/home/sssg/openrvdas/config
ConfigFile=${ConfigDir}/new_mario_485_all.yaml 

# Listen for serial input and write to log file and UDP
# Use listener with config file - no other arguments
# add '-v' to crank up the logging level

logger/listener/listen.py --config_file ${ConfigFile} -v

# Deactivate virtual environment

deactivate
