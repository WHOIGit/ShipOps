#!/usr/bin/env python
# efc 2023 translate UDP message to JSON string

import socket
import json
import sqlite3
import time
import re

while True: 
    # UDP message from datalog
    port = 55602
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg as a string
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = re.split(r'=|,| ', msg)
    #print (string)
    
    # Define variables 
    id = "EM122"
    timestamp = int(time.time())
    waterdepth = float(string[5])
    shipdepth = float(string[6])   
    value = int(waterdepth + shipdepth)
    unit = " m"
            
    # Connect to sqlite db
    dbfile = "armstrong.db"
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()
    table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
    if table == []: # Check if table exists
        print ("Table does not exist")
        
    else: # Write to the sqlite database
        cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id, timestamp, value, unit))
        conn.commit()
        conn.close()