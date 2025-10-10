#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB3
InBaud=9600
OutPort=57303
OutHost=10.100.161.10
Label1=SBE45
Label2=SSW
DateStamp=`date +%Y%m%d`
HourStamp=`date +%H`
LogDir=/home/sssg/openrvdas_support/logs

# Listen for input and write to log file and UDP

logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/at${DateStamp}_${HourStamp}00.${Label1} --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
