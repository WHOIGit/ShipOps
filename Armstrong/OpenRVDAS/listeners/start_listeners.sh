#!/bin/bash

# Serial input
./stream_cnav_serial.sh 2>/dev/null &
./stream_seapath_serial.sh 2>/dev/null &
./stream_posmv_serial.sh 2>/dev/null &
./stream_dps112_serial.sh 2>/dev/null &
./stream_sbe45_serial.sh 2>/dev/null &
./stream_rad_serial.sh 2>/dev/null &
./stream_par_serial.sh 2>/dev/null &
./stream_gyro1_serial.sh 2>/dev/null &
./stream_ssv_serial.sh 2>/dev/null &
./stream_vaisala_port_serial.sh 2>/dev/null &

python3 ./stream_sbe48_serial.py &

# UDP streams
# ./stream_cnav_udp.sh 2>&1 &
# ./stream_dps112_udp.sh 2>&1 &
