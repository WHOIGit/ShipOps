#!/usr/bin/bash

OPENRVDAS_PATH=/opt/openrvdas

# change local path to the directory containing the rs485 UDP config
LOCAL_PATH=/home/sssg/openrvdas_support/config

# Activate virtual environment
echo "Activating virtual environment"
source $OPENRVDAS_PATH/venv/bin/activate

# Start the logger manager
echo "Starting the logger manager for cache writing"
python $OPENRVDAS_PATH/server/logger_manager.py \
    --config $LOCAL_PATH/truewind_readudp.yaml \
    --stderr_file_pattern /home/sssg/openrvdas_support/logs/TW_read \
    -V \
    --no-console 

# Deactivate virtual environment
deactivate