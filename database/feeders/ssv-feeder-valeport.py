#!/usr/bin/env python
# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time

while True: 
    # UDP message from datalog
    port = 55505
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = msg.split()
    #print (string)
    
    # Define variables 
    timestamp = int(time.time())

    id1 = "SSV_Valeport"  
    value1 = float(string[4][:-2])
    unit1 = " m/s"

    id2 = "Temp_Valeport"  
    value2 = float(string[6][:-2])
    unit2 = "°C"    
	
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
