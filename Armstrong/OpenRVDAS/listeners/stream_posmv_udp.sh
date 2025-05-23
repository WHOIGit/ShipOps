#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

# Initialize variables

InPort=55102
OutPort=57102
OutHost=10.100.100.30
Label1=POSMV
Label2=IMU
LogDir=/home/admin_paul.mena/logs

# Listen for input and write to log file and standard out

logger/listener/listen.py --udp ${InPort} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/${Label1}.log --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
