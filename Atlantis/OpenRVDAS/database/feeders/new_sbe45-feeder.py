#!/usr/bin/env python3

import socket
import sqlite3
import time

while True: 
    # UDP message from datalog
    port = 57303
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = msg.split()
    
    # Define variables 
    timestamp = int(time.time())

    id1 = "SBE45Temp"  
    value1 = array[4][:-4]
    unit1 = "°C"

    id2 = "SBE45Sal"  
    value2 = array[6][:-4]
    unit2 = " PSU"    
	
    # Connect to sqlite db
    dbfile = "/var/www/html/database/atlantis.db"
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
