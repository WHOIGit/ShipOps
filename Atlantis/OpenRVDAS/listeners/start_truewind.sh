#!/bin/bash

# Serial input
echo "starting simulate truewind"
./simulate_truewind.sh 2>/dev/null &
sleep 2

echo "starting starting truewind write"
./writeudp_truewind.sh 2>/dev/null &
sleep 2

echo "starting true udp read"
#python3 ./sbe48_serial_processor.py 2>/dev/null &
./readudp_truewind.sh 2>/dev/null &
sleep 2



