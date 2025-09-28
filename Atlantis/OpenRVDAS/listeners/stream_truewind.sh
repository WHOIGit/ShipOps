#!/usr/bin/bash

OPENRVDAS_PATH=/opt/openrvdas
LOCAL_PATH=/home/sssg/openrvdas_support/config

# Activate virtual environment
echo "Activating virtual environment"
source $OPENRVDAS_PATH/venv/bin/activate

# Start the logger manager
echo "Starting the logger manager"
python $OPENRVDAS_PATH/server/logger_manager.py \
    --config $LOCAL_PATH/new_truewind_test.yaml \
    --stderr_file_pattern /home/sssg/openrvdas_support/logs/TW \
    -V \
    --no-console

# Deactivate virtual environment
deactivate
