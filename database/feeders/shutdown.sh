#!/bin/bash
#
#Kill all python feeders for Armstrong website data screens

pkill -f cnav-feeder.py #lat, long, sog, cog from CNAV
pkill -f winch-feeder.py #payout, tension, speed for all three wires
pkill -f gyro-feeder.py #heading
pkill -f speedlog-feeder.py #speedlog from bridge
pkill -f rmb-feeder #eta and range from openCPN

pkill -f wxtp-feeder.py #port vaisala: humidity, temperature, pressure
#wxts-feeder.py & #need to make feeder for stbd vaisala
pkill -f truewind-feeder.py #true wind calculation from datalog
pkill -f par-feeder.py #par from met mast
pkill -f rad-feeder.py #lwr and swr from met mast

pkill -f sbe45-feeder.py #temp and salinity from wet lab
pkill -f sbe48-feeder.py #temp from bow thruster hull
pkill -f flowbot-feeder.py 
pkill -f sonicflow-feeder.py 
pkill -f ssv-feeder.py #calculated from sbe45sal and sbe48temp
pkill -f trans10-feeder.py 
pkill -f trans25-feeder.py

pkill -f knudsen12-feeder.py
#knudsen35-feeder.py & #need to make feeder
pkill -f em710-feeder.py
pkill -f em122-feeder.py
pkill -f ek80-feeder.py

pkill -f wave-feeder.py