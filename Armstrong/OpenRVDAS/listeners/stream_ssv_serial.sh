#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB19
InBaud=19200
OutPort=57319
OutHost=10.100.100.30
Label1=NAV
Label2=GPS
LogDir=/home/admin_paul.mena/logs

# Listen for input and write to log file and UDP

logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/serial_SSV.log --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
