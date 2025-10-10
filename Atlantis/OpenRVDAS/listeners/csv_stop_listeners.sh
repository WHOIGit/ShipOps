#/bin/bash

# kill logger_manager processes
echo "Killing logger_manager processes..."
pkill -f "logger_manager.py.*csv.*yaml"
sleep 1

# kill spawned logger_runner processes
echo "Killing logger_runner processes..."
pkill -f "logger_runner.py"
sleep 1
