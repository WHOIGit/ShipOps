#!/bin/bash

# shut down all running feeder scripts

for i in `ps -ef | grep feeder | grep -v grep | awk '{print $2}'`
do
  echo "killing pid $i"
  kill -9 $i
done
