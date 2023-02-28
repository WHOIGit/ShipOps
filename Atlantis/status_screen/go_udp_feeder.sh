#!/bin/bash
# 
#  Updating for dsLog UPD ports.
#               january 17, 2013  lws and ams
#
#
#added route_feeder.pl and winch_feeder.pl  20130609
#  removed ssv and put it into the sbe45   lstolp 20170820

# database is now written to memory 
database="/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";

# sbe48
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55502 --database $database --udptype SBE48 &
# SWR
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55403 --database $database --udptype shw &
# LWR
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55404 --database $database --udptype lwr &
# FLR
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55503 --database $database --udptype flr &
# Port Vaisala  this parses the data
/usr/lib/cgi-bin/db_driven_data/status_screen/wx_feeder.pl --port 55401 --database $database --udptype wxt &
# STBD Vaisala this parses the data
/usr/lib/cgi-bin/db_driven_data/status_screen/wx_feeder.pl --port 55402 --database $database --udptype wxt &
# gyro
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55103 --database $database --udptype gyro &
# SpeedLog
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55202 --database $database --udptype spd &
# em122 
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55602 --database $database --udptype em122 &
