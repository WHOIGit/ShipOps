#!/bin/bash
# 
#  Updating for dsLog UPD ports.
#               january 17, 2013  lws and ams
#
#  udated for flow, par and swr
#  changed to use a tmpfs file, see if this helps w/ ftp use
#
#

# database 
database="/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";

# gprmc for cog sog dec_lat and dec_lon
#./rmc_feeder.pl &
./rmb_feeder.pl &
# sbe45  need to 
./sbe45_feeder.pl &
# aml feeder
./valeport_feeder.pl &
#shortwave longwave 
./rad_feeder.pl &
## gravimeter  removed 2017
#./gravity_feeder.pl &
# sbe48
./udp_db_feeder.pl --port 55502 --database $database --udptype SBE48 &
# SWR not in use 20201106
#./udp_db_feeder.pl --port 55403 --database $database --udptype met &
# LWR not in use 20201106
#./udp_db_feeder.pl --port 55404 --database $database --udptype lwr &
#PAR
./udp_db_feeder.pl --port 55405 --database $database  --udptype par &
# FLR
./udp_db_feeder.pl --port 55503 --database $database --udptype flr &
# Port Vaisala  this parses the data
./wx_feeder.pl --port 55401 --database $database --udptype wxt &
# STBD Vaisala this parses the data
./wx_feeder.pl --port 55402 --database $database --udptype wxt &
# gyro
./udp_db_feeder.pl --port 55103 --database $database --udptype gyro &
# SpeedLog
./udp_db_feeder.pl --port 55202 --database $database --udptype spd &
# FLOW
./flow_feeder.pl &
##./udp_db_feeder.pl --port 55506 --database $database --udptype flow &
# Winch feeder 
#./winch_feeder.pl


# These feeders should be done after all the other ones are working
# Knudsen Depth
#./depth_feeder.pl &
# EM122 depth feeder
#./em122feeder.pl &
# EK80 Depth feeder
#./ek80feeder.pl &
#sound velocity backup if AML goes down
#./ssv_feeder.pl &
# creates wind for both
#./wind_feeder.pl &
# wamos wind feeder
#./wamos_wind_udp.pl &

