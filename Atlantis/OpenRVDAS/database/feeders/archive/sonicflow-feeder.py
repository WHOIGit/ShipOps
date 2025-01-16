#!/usr/bin/env python

import socket
import sqlite3
import time
import numbers

while True: 
    # UDP message from datalog
    port = 55666
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = msg.split()
    
    if array[4] == 'T':
        time.sleep(0.5)
    
    else:

        # Define variables 
        id = "SonicFlow"  
        timestamp = int(time.time())
        value = array[4]
        unit = " L/m"

        # Connect to sqlite db
        dbfile = "/var/www/html/database/armstrong.db"
        conn = sqlite3.connect(dbfile)
        cursor = conn.cursor()
        table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
        if table == []: # Check if table exists
            print ("Table does not exist")
        
        else: # Write to the sqlite database
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id, timestamp, value, unit))
            conn.commit()
            conn.close()
else:
    time.sleep(1)
