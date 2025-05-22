#/bin/bash

# shut down all listener processes

for i in `ps -ef | grep listen.py | grep -v grep | awk '{print $2}'`
do
    echo "killing pid $i"
    kill -9 $i
done

# Kill the RS485 post-processor(s)
for i in `ps -ef | grep atlantis_rs485_udp_processor.py | grep -v grep | awk '{print $2}'`
do
    echo "killing pid ${i}"
    kill -9 ${i}
done
