#!/usr/bin/bash

OPENRVDAS_PATH=/opt/openrvdas
LOCAL_PATH=/home/sssg/openrvdas_support/config

# Activate virtual environment
echo "Activating virtual environment"
source $OPENRVDAS_PATH/venv/bin/activate

# Start the logger manager with simplified configuration
echo "Starting the logger manager with simplified CNAV configuration"
python $OPENRVDAS_PATH/server/logger_manager.py \
    --config $LOCAL_PATH/cnav_simple_csv.yaml \

# Deactivate virtual environment
deactivate
