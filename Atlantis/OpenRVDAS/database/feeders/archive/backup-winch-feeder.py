#!/usr/bin/env python

import socket
import sqlite3
import time
import datetime

while True:
    # UDP message from datalog
    port = 55801
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))

    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    try:
        msg = data.decode()
        array = msg.split(",")

        # Prevent index errors
        if len(array) == 6:

            # Define variables 
            timestamp = int(time.time())
            dateTime = datetime.datetime.now()
            
            id1 = "Tension"  
            value1 = int(float(array[2]))
            unit1 = "lbs"
    
            id2 = "Payout"  
            value2 = float(array[4])
            unit2 = "m"
    
            id3 = "Speed"  
            value3 = float(array[3])
            unit3 = "m/min"
    
            # Connect to sqlite db
            dbfile = "/var/www/html/database/armstrong.db"
            conn = sqlite3.connect(dbfile)
            cursor = conn.cursor()
            table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
            winch = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='winch'; """).fetchall()
 
            if table == []: # Check if table exists
                print ("Table does not exist")
        
            else: # Write to the sqlite database
                
                cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value1, unit1))
        
                cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id2, timestamp, value2, unit2))
                  
                cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id3, timestamp, value3, unit3)) 
                
                if winch == []: # Check if table exists
                    print ("Table does not exist")
            
                else:
                    cursor.execute("DELETE FROM winch WHERE (timestamp <= datetime('now', '-1 day'))")
                    
                    cursor.execute("INSERT INTO winch (timestamp, tension, payout, speed) VALUES (?, ?, ?, ?)", (dateTime, value1, value2, value3))
                    
                    conn.commit()
                    conn.close()
                
                    time.sleep(1) 
        
    except UnicodeDecodeError: # Handle decode error when winches are switched
        
        time.sleep(1)
