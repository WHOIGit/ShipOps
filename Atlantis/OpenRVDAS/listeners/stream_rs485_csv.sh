#!/usr/bin/bash

OPENRVDAS_PATH=/opt/openrvdas
LOCAL_PATH=/home/sssg/openrvdas_support/config

# Activate virtual environment
echo "Activating virtual environment"
source $OPENRVDAS_PATH/venv/bin/activate

# Start the logger manager first
echo "Starting the logger manager"
python $OPENRVDAS_PATH/server/logger_manager.py \
    --config $LOCAL_PATH/rs485_csv.yaml \
    --no-console &

# Wait a few seconds for logger manager to initialize
echo "Waiting for logger manager to initialize..."
sleep 2

# Then simulate data on a port
echo "Starting data simulation on UDP port 57504"
python $OPENRVDAS_PATH/logger/utils/simulate_data.py \
    --config $LOCAL_PATH/simulate_rs485.yaml \
    --record_format "{prefix1} {timestamp} {prefix2} {record}"

# Deactivate virtual environment
deactivate
