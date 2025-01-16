#!/bin/bash

for i in `ps -ef | grep simulate_data | grep -v grep | awk '{ print $2 }'`
do
   echo "killing pid ${i}"
   kill -9 ${i}
done

for i in `ps -ef | grep underway_data_monitor.py | grep -v grep | awk '{ print $2 }'`
do
   echo "killing pid ${i}"
   kill -9 ${i}
done
