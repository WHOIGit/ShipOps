#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB22
InBaud=9600
Interval=8
OutPort=57322
OutHost=10.100.100.30
Label1=SBE48
Label2=SSW
LogDir=/home/admin_paul.mena/logs

# Listen for input and write to log file and UDP

# Include read interval
#logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --interval ${Interval} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/serial_SBE48.log --write_udp ${OutHost}:${OutPort} -vv

# Read from serial port
logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/serial_SBE48.log --write_udp ${OutHost}:${OutPort} -vv

# Deactivate virtual environment

deactivate
