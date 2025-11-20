#!/bin/bash

python test_cnav-feeder.py 2>/dev/null &
python test_gyro-feeder.py 2>/dev/null &
python test_sbe48-feeder.py 2>/dev/null &
python test_sbe45-feeder.py 2>/dev/null &

# the Underway Data Status Screen only uses the Starboard Vaisala
# python test_wxtp-feeder.py 2>/dev/null &
