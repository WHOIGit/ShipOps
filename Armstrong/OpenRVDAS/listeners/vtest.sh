#! /bin/bash

cat /dev/ttyUSB4 > /home/admin_paul.mena/tmp/v.data &

while true
do
  echo -en 'PR0\r\n' > /dev/ttyUSB4
  sleep 1
  echo -en 'SR0\r\n' > /dev/ttyUSB4
  sleep 1
# echo -en '$3RD\r' > /dev/ttyUSB4
  sleep 5
done
