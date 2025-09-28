#!/bin/bash

# show python processes owned by user sssg
# ps -ef | grep listen.py | grep -v grep 
ps -ef | grep python | grep sssg | grep -v grep

# show RS485 post-processor
# ps -ef | grep atlantis_rs485_udp_processor.py | grep -v grep
