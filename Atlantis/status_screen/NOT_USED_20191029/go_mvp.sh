#!/bin/bash
# 
#  Updating for dsLog UPD ports.
#               january 17, 2013  lws and ams
#
#
#added route_feeder.pl and winch_feeder.pl  20130609

# database 
database="/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/mvp.db";

# GPGGA
/usr/lib/cgi-bin/db_driven_data/status_screen/mvp_udp_feeder.pl --port 55003 --database $database --udptype gps &


# ZDA
/usr/lib/cgi-bin/db_driven_data/status_screen/mvp_udp_feeder.pl --port 55002 --database $database --udptype zda &

#  depth
/usr/lib/cgi-bin/db_driven_data/status_screen/mvp_udp_feeder.pl --port 55609 --database $database --udptype depth &

# SpeedLog
/usr/lib/cgi-bin/db_driven_data/status_screen/mvp_udp_feeder.pl --port 55203 --database $database --udptype spd &

