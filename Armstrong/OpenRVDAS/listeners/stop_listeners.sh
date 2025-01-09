#!/bin/bash

for i in `ps -ef | grep listen.py | grep -v grep | awk '{print $2}'`
do
    echo "killing pid ${i}"
    kill -9 ${i}
done
