#!/usr/bin/env python3

import socket
import sqlite3
import time

while True: 
    # UDP message from datalog
    port = 55502
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = msg.split()
    
    # Handle every other msg being blank
    if len(array) > 4:
    
        # Define variables 
        id = "SBE48Temp"  
        timestamp = int(time.time())
        value = array[5][:-4] 
        unit = "°C"    
    
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