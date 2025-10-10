#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB0
InBaud=4800
OutPort=57300
OutHost=10.100.161.10
Label1=NAV
Label2=GPS
DateStamp=`date +%Y%m%d`
HourStamp=`date +%H`
LogDir=/home/sssg/openrvdas_support/logs

# Listen for input and write to log file and UDP

logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/at${DateStamp}_${HourStamp}00.${Label2} --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
