#!/bin/bash

python new_cnav-feeder.py 2>/dev/null &
python new_gyro-feeder.py 2>/dev/null &
python new_sbe48-feeder.py 2>/dev/null &
python new_sbe45-feeder.py 2>/dev/null &
python new_wxts-feeder.py 2>/dev/null &

# the Underway Data Status Screen only uses the Starboard Vaisala
# python new_wxtp-feeder.py 2>/dev/null &
