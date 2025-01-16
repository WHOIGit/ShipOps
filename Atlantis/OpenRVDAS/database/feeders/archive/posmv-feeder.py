#!/usr/bin/env python

import socket
import sqlite3
import time
import re
import datetime

while True: 

    # Define UDP port number
    port = 55102 
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg as array
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = re.split(r'=|,| ', msg) 
    
    if array[4] == "$PASHR":
    
        # Define variables 
        timestamp = int(time.time())
        dateTime = datetime.datetime.now()
    
        id1 = "Roll"  
        value1 = array[8]
        unit1 = "°"
    
        id2 = "Pitch"  
        value2 = array[9]
        unit2 = "°"
    
        id3 = "Heave"  
        value3 = array[10]
        unit3 = "°"
    
        # Connect to sqlite db
        dbfile = "/var/www/html/database/armstrong.db"
        conn = sqlite3.connect(dbfile)
        cursor = conn.cursor()
        table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
        posmv = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='posmv'; """).fetchall()
 
        if table == []: # Check if table exists
            print ("Table does not exist")
        
        else: # Write to the sqlite database
                
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value1, unit1))
        
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id2, timestamp, value2, unit2))
                  
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id3, timestamp, value3, unit3))
        
            if posmv == []: # Check if table exists
                print ("Table does not exist")
                    
            else:
                cursor.execute("DELETE FROM posmv WHERE (timestamp <= datetime('now', '-1 day'))")
                    
                cursor.execute("INSERT INTO posmv (timestamp, roll, pitch, heave) VALUES (?, ?, ?, ?)", (dateTime, value1, value2, value3))
                    
                conn.commit()
                conn.close()
                
                time.sleep(0.3) 