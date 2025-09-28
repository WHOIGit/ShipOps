#!/bin/bash

echo "starting SBE45"
./stream_sbe45_csv.sh 2>/dev/null &
sleep 2

echo "starting SBE48"
./stream_sbe48_csv.sh 2>/dev/null &
sleep 2

echo "starting GYRO1"
./stream_gyro1_csv.sh 2>/dev/null &
sleep 2

echo "starting CNAV"
./stream_cnav_csv.sh 2>/dev/null &
sleep 2

#echo "starting RS485"
#./stream_rs485_csv.sh 2>/dev/null &
#sleep 2

echo "starting CSV ALL"
./stream_csv_all.sh 2>/dev/null &
sleep 2
