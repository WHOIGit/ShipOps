#!/bin/bash

for i in `ps -ef | grep feeder | grep -v grep | awk '{print $2}'`
do
  echo "killing pid ${i}"
  kill ${i}
done
