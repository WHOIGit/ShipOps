#!/bin/bash
# 
#  Updating for dsLog UPD ports.
#               january 17, 2013  lws and ams
#
#

# database 
database="/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";



# gprmc for cog sog dec_lat and dec_lon
./rmc_feeder.pl &
#./rmb_feeder.pl &

# sbe45  need to 
./sbe45_feeder.pl &


# sbe48
#./udp_db_feeder.pl --port 55503 --database $database --udptype SBE48 &
# FLR
./udp_db_feeder.pl --port 55506 --database $database --udptype flr &
# Port Vaisala  this parses the data
./wx_feeder.pl --port 55401 --database $database --udptype wxt &
# gyro
./udp_db_feeder.pl --port 55103 --database $database --udptype gyro &
# flow
 ./flow_feeder.pl &


# These feeders should be done after all the other ones are working
# Knudsen Depth
#./tidepth_feeder.pl &
# sound velocity
#./ssv_feeder.pl &
# creates wind for both
./wind_feeder.pl &

