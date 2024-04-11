#!/usr/bin/env python
# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time

while True:
    # UDP message from datalog
    port = 55801
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))

    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    try:
        msg = data.decode()
        string = msg.split(",")
        #print (string)

        # Prevent index errors
        if len(string) == 6:

            # Define variables 
            timestamp = int(time.time())
            
            id1 = "Tension"  
            value1 = int(float(string[2]))
            unit1 = "lbs"
    
            id2 = "Payout"  
            value2 = float(string[4])
            unit2 = "m"
    
            id3 = "Speed"  
            value3 = float(string[3])
            unit3 = "m/min"
    
            # Connect to sqlite db
            dbfile = "armstrong.db"
            conn = sqlite3.connect(dbfile)
            cursor = conn.cursor()
            table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
            if table == []: # Check if table exists
                print ("Table does not exist")
        
            else: # Write to the sqlite database
                
                cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value1, unit1))
        
                cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id2, timestamp, value2, unit2))
                  
                cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id3, timestamp, value3, unit3))
        
                conn.commit()
                conn.close()
                
                time.sleep(0.5) 
        
    except UnicodeDecodeError: # Handle decode error when winches are switched
        time.sleep(0.5)