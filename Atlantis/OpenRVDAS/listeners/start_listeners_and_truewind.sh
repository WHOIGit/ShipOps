#!/bin/bash

# Serial input
echo "starting CNAV"
./stream_cnav_serial_filewriter.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
sleep 2

echo "starting GYRO1"
./stream_gyro1_serial_filewriter.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
sleep 2

echo "starting SBE48"
#python3 ./sbe48_serial_processor.py >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
./stream_sbe48_serial_filewriter.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
sleep 2

echo "starting SBE45"
./stream_sbe45_serial_filewriter.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
sleep 2

# echo "starting the RS485 loop"
# ./stream_485_serial_notransform.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &

echo "starting simulate truewind"
./simulate_truewind.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
sleep 2

echo "starting RS485 UDP processor"
python atlantis_iso8601_rs485_udp_processor.py >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
sleep 2

echo "starting starting truewind write"
./writeudp_truewind.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &
sleep 2

echo "starting true udp read"
./readudp_truewind.sh >> /home/sssg/openrvdas_support/logs/truewind_read_$(date -u +%Y%m%d).log 2>&1 &