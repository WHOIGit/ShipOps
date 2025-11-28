#!/bin/bash
#
#Start python feeders for Atlantis True Wind testing

python ./test_cnav-feeder-rmc.py 2>/dev/null &
python ./test_gyro-feeder.py 2>/dev/null & 
python ./test_sbe45-feeder.py 2>/dev/null &
python ./test_sbe48-feeder.py 2>/dev/null &

#./wxtp-feeder.py & #port vaisala: humidity, temperature, pressure
#./wxts-feeder.py & #need to make feeder for stbd vaisala
#./truewind-feeder.py & #true wind calculation from datalog
