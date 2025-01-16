#!/usr/bin/env python3

import socket
import sqlite3
import time
import re

while True:
    # UDP message from datalog
    port = 55001
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))

    # Receive and store udp msg
    data, addr = sock.recvfrom(1024) 
    msg = data.decode()
    array = re.split(r'=|,| ', msg)
    
    # Get messages for lat/lon
    if array[4] == "$GPGGA":
        
        # Define variables 
        timestamp = int(time.time())
            
        id1 = "Latitude"  
        latdeg = int(float(array[6][:-9]))
        latmin = array[6][2:]
        value1 = str(latdeg) + "° " + str(latmin) +"&apos;"
        unit1 = array[7]
        
        id2 = "Longitude"  
        londeg = int(float(array[8][:-9]))
        lonmin = array[8][3:]       
        value2 = str(londeg) + "° " + str(lonmin) + "&apos;"
        unit2 = array[9]
        
        # Connect to sqlite db 
        dbfile = "/var/www/html/database/armstrong.db"
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

    # Get messages for COG/SOG
    elif array[4] == "$GPVTG":
        
        # Define variables 
        timestamp = int(time.time())
        
        jsVariable3 = "cog"
        id3 = "COG"  
        value3 = array[5]
        unit3 = "°"
 
        jsVariable4 = "sog"
        id4 = "SOG"  
        value4 = array[9]
        unit4 = " kts"   
        
        # Connect to sqlite db 
        dbfile = "/var/www/html/database/armstrong.db"
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
