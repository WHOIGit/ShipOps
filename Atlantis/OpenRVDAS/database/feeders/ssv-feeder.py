#!/usr/bin/python3
# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time

while True: 
    # UDP message from datalog
    port = 55504
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = msg.split()
    #print (string)
    
    # Define variables 
    id = "SSV" 
    timestamp = int(time.time())
    value = int(float(string[1]))
    unit = " m/s"
	
    # Connect to sqlite db
    dbfile = "atlantis.db"
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()
    table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
    if table == []: # Check if table exists
        print ("Table does not exist")
        
    else: # Write to the sqlite database
        cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id, timestamp, value, unit))
        conn.commit()
        conn.close()
