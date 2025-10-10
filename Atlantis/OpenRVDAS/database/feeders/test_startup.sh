#!/bin/bash
#
#Start python feeders for Atlantis True Wind testing

python ./new_cnav-feeder.py & #lat, long, sog, cog from CNAV
#/winch-feeder.py & #payout, tension, speed for all three wires
python ./new_gyro-feeder.py & #heading

#./wxtp-feeder.py & #port vaisala: humidity, temperature, pressure
#./wxts-feeder.py & #need to make feeder for stbd vaisala
#./truewind-feeder.py & #true wind calculation from datalog
