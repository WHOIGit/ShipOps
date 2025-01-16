#!/bin/bash

# start up python feeder scripts

python3 ./new_cnav-feeder.py &
python3 ./new_dps-feeder.py &
python3 ./new_gyro1-feeder.py &
python3 ./new_flowbot-feeder.py &
python3 ./new_par-feeder.py &
python3 ./new_rad-feeder.py &
python3 ./new_sbe45-feeder.py &
python3 ./new_sbe48-feeder.py &
python3 ./new_ssv-feeder-valeport.py &
python3 ./new_wxtp-feeder.py &
python3 ./new_winch-feeder.py &
