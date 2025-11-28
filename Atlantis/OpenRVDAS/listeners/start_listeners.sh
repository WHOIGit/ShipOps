#!/bin/bash

# Serial input
echo "starting CNAV"
./stream_cnav_serial_filewriter.sh 2>/dev/null &
sleep 2

echo "starting GYRO1"
./stream_gyro1_serial_filewriter.sh 2>/dev/null &
sleep 2

echo "starting SBE48"
./stream_sbe48_serial_filewriter.sh 2>/dev/null &
sleep 2

echo "starting SBE45"
./stream_sbe45_serial_filewriter.sh 2>/dev/null &
sleep 2

