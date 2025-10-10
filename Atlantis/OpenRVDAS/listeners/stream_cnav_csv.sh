#!/usr/bin/bash

OPENRVDAS_PATH=/opt/openrvdas
LOCAL_PATH=/home/sssg/openrvdas_support/config

# Activate virtual environment
echo "Activating virtual environment"
source $OPENRVDAS_PATH/venv/bin/activate

# Start the logger manager
echo "Starting the logger manager"
python $OPENRVDAS_PATH/server/logger_manager.py \
    --config $LOCAL_PATH/cnav_csv.yaml \
    --no-console

#python $OPENRVDAS_PATH/logger/listener/listen.py \
#    --config_file $LOCAL_PATH/test_cnav.yaml

# Deactivate virtual environment
deactivate
