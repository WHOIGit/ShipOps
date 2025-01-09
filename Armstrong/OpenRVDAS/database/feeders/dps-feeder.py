#!/usr/bin/env python3

import socket
import time
import re
import sqlite3

while True: 
    # UDP message from datalog
    port = 55006
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg as array
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = re.split(r'=|,| ', msg)   # Use "re" module if splitting a message by 2 deliminators         

    # Get messages for COG/SOG
    if array[4] == "$GPVTG":
        
        # Define variables 
        timestamp = int(time.time())
        
        jsVariable1 = "dpscog"
        id1 = "DPSCOG"  
        value1 = array[5]
        unit1 = "°"  
        
        # Connect to sqlite db 
        dbfile = "/var/www/html/database/armstrong.db"
        conn = sqlite3.connect(dbfile)
        cursor = conn.cursor()
        table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
        if table == []: # Check if table exists
            print ("Table does not exist")
        
        else: # Write to the sqlite database
 
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value1, unit1))
 
            conn.commit()
            conn.close()
