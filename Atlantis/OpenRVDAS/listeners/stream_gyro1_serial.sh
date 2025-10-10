#!/bin/bash

# Activate OpenRVDAS virtual environment

cd /opt/openrvdas
source venv/bin/activate

InPort=/dev/ttyUSB1
InBaud=4800
OutPort=57301
OutHost=10.100.161.10
Label1=GYRO
Label2=NAV
DateStamp=`date +%Y%m%d`
HourStamp=`date +%H`
LogDir=/home/sssg/openrvdas_support/logs

# Listen for input and write to log file and UDP

logger/listener/listen.py --serial port=${InPort},baudrate=${InBaud} --transform_prefix ${Label1} --time_format '%Y/%m/%d %H:%M:%S.%f' --transform_timestamp --transform_prefix ${Label2} --write_file ${LogDir}/at${DateStamp}_${HourStamp}00.HDT --write_udp ${OutHost}:${OutPort}

# Deactivate virtual environment

deactivate
