#!/bin/bash

# Serial input
echo "starting CNAV"
./stream_cnav_serial_filewriter.sh 2>/dev/null &
sleep 2

echo "starting GYRO1"
./stream_gyro1_serial_filewriter.sh 2>/dev/null &
sleep 2

echo "starting SBE48"
#python3 ./sbe48_serial_processor.py 2>/dev/null &
./stream_sbe48_serial_filewriter.sh 2>/dev/null &
sleep 2

echo "starting SBE45"
./stream_sbe45_serial_filewriter.sh 2>/dev/null &
sleep 2

# echo "starting the RS485 loop"
# ./stream_485_serial_notransform.sh 2>/dev/null &
# sleep 2
# python atlantis_iso8601_rs485_udp_processor.py 2>/dev/null &

