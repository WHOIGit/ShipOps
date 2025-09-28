#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

ConfigDir=/home/sssg/openrvdas_support/config
ConfigFile=${ConfigDir}/485_all_notransform.yaml

# Listen for serial input and write to log file and UDP
# Use listener with config file - no other arguments
# add '-v' to crank up the logging level

logger/listener/listen.py --config_file ${ConfigFile} -v

# Deactivate virtual environment

deactivate
