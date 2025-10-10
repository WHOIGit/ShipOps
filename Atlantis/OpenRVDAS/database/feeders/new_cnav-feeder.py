#!/usr/bin/python3
# efc 2023 store UDP message in sqlite database

import socket
import json
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
    string = re.split(r'=|,| ', msg)
    #print (string)
    
    # Get messages for lat/lon
    if string[3] == "$GPGGA":
        
        # Define variables 
        timestamp = int(time.time())
            
        id1 = "Dec_Lat"  
        latdeg = int(float(string[5][:-8]))
        latmin = string[5][1:]
        value1 = str(latdeg) + "° " + str(latmin) +"&apos;"
        unit1 = string[6]
        
        id2 = "Dec_Long"  
        londeg = int(float(string[7][:-8]))
        lonmin = string[7][2:]       
        value2 = str(londeg) + "° " + str(lonmin) + "&apos;"
        unit2 = string[8]
        
        # Connect to sqlite db 
        dbfile = "atlantis.db"        
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
    elif string[3] == "$GPVTG":
        
        # Define variables 
        timestamp = int(time.time())
        
        jsVariable3 = "cog"
        id3 = "COG"  
        value3 = string[4]
        unit3 = "°"
 
        jsVariable4 = "sog"
        id4 = "SOG"  
        value4 = string[8]
        unit4 = " kts"   
        
        # Connect to sqlite db 
        dbfile = "atlantis.db"        
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
