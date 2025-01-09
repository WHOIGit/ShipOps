#!/usr/bin/env python3

import socket
import sqlite3
import time

while True: 
    # UDP message from datalog
    port = 55405
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg as a array
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = msg.split()
    
    # Define variables 
    timestamp = int(time.time()) 
    id = "PAR"
    value = int(float(array[5][:-1]))
    unit = " uE"
	
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