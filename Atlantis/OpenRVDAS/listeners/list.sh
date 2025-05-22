#!/bin/bash

# show listener processes
ps -ef | grep listen.py | grep -v grep 

# show RS485 post-processor
ps -ef | grep atlantis_rs485_udp_processor.py | grep -v grep
