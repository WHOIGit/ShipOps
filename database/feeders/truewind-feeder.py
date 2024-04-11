#!/usr/bin/env python
# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time

while True: 
    # UDP message from datalog
    port = 56000
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = msg.split()
    #print (string)
    
    if string[5] == "NAN" or string[4] == "NAN" or string[7] == "NAN" or string[6] == "NAN" : 
        time.sleep = 0.5 
        
    else :
         
        # Define variables 
        timestamp = int(time.time())
        
        jsVariable1 = "windsp"
        id1 = "WindSpeed"  
        ms = ( float(string[5]) + float(string[7]) ) / 2
        value1 = int(ms * 1.944)
        unit1 = " kts"
 
        jsVariable2 = "winddir"
        id2 = "WindDirection"  
        value2 = int(( float(string[4]) + float(string[6]) ) / 2 )
        unit2 = "°"

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
        
            conn.commit()
            conn.close()
