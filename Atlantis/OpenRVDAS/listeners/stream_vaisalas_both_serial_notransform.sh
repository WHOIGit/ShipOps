#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB4
InBaud=9600
OutPort=57304
OutHost=10.100.100.30
Label1=WXTP
Label2=MET
LogDir=/home/admin_paul.mena/logs
ConfigDir=/home/admin_paul.mena/config
ConfigFile=${ConfigDir}/vaisalas_both_notransform.yaml

# Listen for serial input and write to log file and UDP
# Use listener with config file - no other arguments
# add '-v' to crank up the logging level

logger/listener/listen.py --config_file ${ConfigFile} 

# Deactivate virtual environment

deactivate
