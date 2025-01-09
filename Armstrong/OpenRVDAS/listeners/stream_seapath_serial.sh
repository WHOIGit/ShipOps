#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB1
InBaud=9600
OutPort=57301
OutHost=10.100.100.30
Label1=NAV
Label2=GPS
LogDir=/home/admin_paul.mena/logs

# Listen for input and write to log file and UDP

# logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/serial_Seapath.log --write_udp ${OutHost}:${OutPort}

# Output to UDP without transformation
logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --write_file ${LogDir}/serial_Seapath.log --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
