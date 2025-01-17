#!/bin/bash

# start up all configured listener processes

cd ~/listeners
./stream_cnav_serial.sh 2>/dev/null &
./stream_rad_serial.sh 2>/dev/null &
./stream_gyro1_serial.sh 2>/dev/null &
./stream_sbe45_serial.sh 2>/dev/null &
./stream_485_serial.sh 2>/dev/null &
./stream_flow_serial.sh 2>/dev/null &
./stream_xmiss_serial.sh 2>/dev/null &
./stream_gravity_serial.sh 2>/dev/null &

# python script for sbe48

python3 ./stream_sbe48_serial.py 2>/dev/null &
