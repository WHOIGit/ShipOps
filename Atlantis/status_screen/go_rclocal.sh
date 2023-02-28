#!/bin/bash
# 
#  Updating for dsLog UPD ports.
#               january 17, 2013  lws and ams
#
#
#added route_feeder.pl and winch_feeder.pl  20130609

# database 
database="/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";

# gprmc for cog sog dec_lat and dec_lon
/usr/lib/cgi-bin/db_driven_data/status_screen/rmc_feeder.pl &
/usr/lib/cgi-bin/db_driven_data/status_screen/rmb_feeder.pl &
# sbe45  need to 
/usr/lib/cgi-bin/db_driven_data/status_screen/sbe45_feeder.pl &

# sbe48
/usr/lib/cgi-bin/db_driven_data/status_screen/udp_db_feeder.pl --port 55502 --database $database --udptype SBE48 &
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
# trans
/usr/lib/cgi-bin/db_driven_data/status_screen/trans_feeder.pl &


# These feeders should be done after all the other ones are working
# Knudsen Depth 3260 or 320
# /usr/lib/cgi-bin/db_driven_data/status_screen/depth_feeder.3260.pl &
#/usr/lib/cgi-bin/db_driven_data/status_screen/depth_feeder.320BR.pl &

# sound velocity
#/usr/lib/cgi-bin/db_driven_data/status_screen/ssw_feeder.pl &
/usr/lib/cgi-bin/db_driven_data/status_screen/redirect_svt_from_UDP.pl &

# creates wind for both
/usr/lib/cgi-bin/db_driven_data/status_screen/wind_feeder.pl &

# route feeder 
/usr/lib/cgi-bin/db_driven_data/status_screen/route_feeder.pl &
# winch feeder 
/usr/lib/cgi-bin/db_driven_data/status_screen/winch_feeder.pl &
# lwr and swr feeder
/usr/lib/cgi-bin/db_driven_data/status_screen/rad_feeder.pl &

#flowbot
/usr/bin/perl /usr/lib/cgi-bin/db_driven_data/status_screen/flowbot_feeder.pl

#alvin
/usr/bin/perl /usr/lib/cgi-bin/db_driven_data/status_screen/55908_feeder.pl

# Network UDP Relays (started by /etc/rc.local, not here)
# GPS
# /usr/local/bin/udp-broadcast-relay.whoi -f 96 55000 eth0 eth2 eth3 &
# NAAMES special
# /usr/local/bin/udp-broadcast-relay.whoi -f 99 55900 eth0 eth2 &
# Gyro. Use 55101 if you want Phins heading
# /usr/local/bin/udp-broadcast-relay.whoi -f 83 55100 eth0 eth2 eth3 &

