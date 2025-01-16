#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB8
InBaud=57600
OutPort=57308
OutHost=10.100.128.22
Label1=NAV
Label2=GPS
LogDir=/home/admin_paul.mena/logs

# Listen for input and write to log file and UDP

logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/serial_gravity.log --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
