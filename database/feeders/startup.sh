#!/bin/bash
#
#Start python feeders for Armstrong website data screens

./cnav-feeder.py & #lat, long, sog, cog from CNAV
./winch-feeder.py & #payout, tension, speed for all three wires
./gyro-feeder.py & #heading
./speedlog-feeder.py & #speedlog from bridge
./rmb-feeder.py & #eta and range from openCPN

./wxtp-feeder.py & #port vaisala: humidity, temperature, pressure
#./wxts-feeder.py & #need to make feeder for stbd vaisala
./truewind-feeder.py & #true wind calculation from datalog
./par-feeder.py & #par from met mast
./rad-feeder.py & #lwr and swr from met mast

./sbe45-feeder.py & #temp and salinity from wet lab
./sbe48-feeder.py & #temp from bow thruster hull
./flowbot-feeder.py & 
./sonicflow-feeder.py & 
#./ssv-feeder.py & #calculated from sbe45sal and sbe48temp
./ssv-feeder-valeport.py &
./trans10-feeder.py & 
./trans25-feeder.py &

./knudsen12-feeder.py &
#./knudsen35-feeder.py & #need to make feeder
./em710-feeder.py &
./em122-feeder.py &
./ek80-feeder.py &

./wave-feeder.py& 
