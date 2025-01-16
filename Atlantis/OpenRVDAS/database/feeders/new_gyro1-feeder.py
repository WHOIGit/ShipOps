#!/usr/bin/env python3

import socket
import sqlite3
import time
import re

while True: 
    # UDP message from datalog
    port = 57301
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = re.split(r'=|,| ', msg)
    
    if array[4] == "$HEHDT":
    
        # Define variables 
        id = "Heading"  
        timestamp = int(time.time())
        value = int(float(array[5]))
        unit = "°"

        # Connect to sqlite db
        dbfile = "/var/www/html/database/atlantis.db"
        conn = sqlite3.connect(dbfile)
        cursor = conn.cursor()
        table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
        if table == []: # Check if table exists
            print ("Table does not exist")
        
        else: # Write to the sqlite database
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id, timestamp, value, unit))
            conn.commit()
            conn.close()
