#!/bin/bash

echo "Stopping serial listeners"
./stop_listeners.sh

echo "Stopping CSV listeners"
./csv_stop_listeners.sh
