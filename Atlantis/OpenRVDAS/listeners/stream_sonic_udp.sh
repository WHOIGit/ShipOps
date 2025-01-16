#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

# Initialize variables

InPort=55665
OutPort=57665
OutHost=10.100.100.30
Label1=Flow_Sonic
Label2=SSW
LogDir=/home/admin_paul.mena/logs

# Listen for input and write to log file and standard out

logger/listener/listen.py --udp ${InPort} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/${Label1}.log --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
