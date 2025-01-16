#!/bin/bash

source /opt/openrvdas/venv/bin/activate

echo "starting up simulate_data.py"

cd /opt/openrvdas/logger/utils && python3 simulate_data.py --config ~/my_git/underway_rvdas/test_data/status_screen/sim_status_screen_data.yaml &

sleep 5
echo "starting up feeder scripts"

cd ~/my_git/underway_rvdas/database/feeders && ./startup.sh 

sleep 5
echo "Running underway_data_monitor.py"

cd ~/my_git/underway_rvdas && python3 underway_data_monitor.py --ship test &

deactivate
