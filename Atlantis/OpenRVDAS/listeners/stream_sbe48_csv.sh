#!/usr/bin/bash

OPENRVDAS_PATH=/opt/openrvdas
LOCAL_PATH=/home/sssg/openrvdas_support
CONFIG_PATH=$LOCAL_PATH/config
LISTENERS_PATH=$LOCAL_PATH/listeners

# Activate virtual environment
echo "Activating virtual environment"
source $OPENRVDAS_PATH/venv/bin/activate

# # Start the SBE48 processor
# echo "Starting the SBE48 processor"
# python $LISTENERS_PATH/sbe48_serial_processor.py &

# Start the logger manager
echo "Starting the logger manager"
python $OPENRVDAS_PATH/server/logger_manager.py \
    --config $CONFIG_PATH/sbe48_csv.yaml \
    --no-console

# Deactivate virtual environment
deactivate
