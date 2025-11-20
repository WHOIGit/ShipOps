#!/usr/bin/env python3
#source of SOG & COG is now from the RMC string (different from Armstrong)

import socket
import sqlite3
import time
import re

while True:
    # UDP message from datalog
    port = 57300
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))

    # Receive and store udp msg
    data, addr = sock.recvfrom(1024) 
    msg = data.decode()
    array = re.split(r'=|,| ', msg)
    
    # Get messages for lat/lon
    if array[3] == "$GPGGA":
        
        # Define variables 
        timestamp = int(time.time())
            
        id1 = "Latitude"  
        latdeg = int(float(array[5][:-9]))
        latmin = array[5][2:]
        value1 = str(latdeg) + "° " + str(latmin) +"&apos;"
        unit1 = array[6]
        
        id2 = "Longitude"  
        londeg = int(float(array[7][:-9]))
        lonmin = array[7][3:]       
        value2 = str(londeg) + "° " + str(lonmin) + "&apos;"
        unit2 = array[8]
        
        # Connect to sqlite db 
        dbfile = "/var/www/html/database/atlantis_test.db"
        conn = sqlite3.connect(dbfile)
        cursor = conn.cursor()
        table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
        if table == []: # Check if table exists
            print ("Table does not exist")
        
        else: # Write to to the sqlite database
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value1, unit1))
        
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id2, timestamp, value2, unit2))
        
            conn.commit()
            conn.close()

#USE RMC FOR COG & SOG
    # Get messages for COG/SOG
    elif array[3] == "$GPRMC":
        
        # Define variables 
        timestamp = int(time.time())
        
        jsVariable3 = "cog"
        id3 = "COG"  
        value3 = array[11]
        unit3 = "°"
 
        jsVariable4 = "sog"
        id4 = "SOG"  
        value4 = array[10]
        unit4 = " kts"   
        
        # Connect to sqlite db 
        dbfile = "/var/www/html/database/atlantis_test.db"
        conn = sqlite3.connect(dbfile)
        cursor = conn.cursor()
        table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
        if table == []: # Check if table exists
            print ("Table does not exist")
        
        else: # Write to the sqlite database
 
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id3, timestamp, value3, unit3))

            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id4, timestamp, value4, unit4))
 
            conn.commit()
            conn.close()
