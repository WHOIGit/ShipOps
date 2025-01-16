#!/usr/bin/python3
# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time
import re

while True: 
    # UDP message from datalog
    port = 55608
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg as a string
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = re.split(r'=|,| ', msg)
    #print (string)
   
    # Define variables 
    id1 = "Knudsen35"
    timestamp = int(time.time())
    value = string[5]
    unit = " m"

    # Define variables 
    id2 = "Knudsen12" 
    timestamp = int(time.time())    
    value = string[8]
    unit = " m"

    # Connect to sqlite db
    dbfile = "atlantis.db"
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()
    table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
    if table == []: # Check if table exists
        print ("Table does not exist")
        
    else: # Write to the sqlite database
        cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value, unit))
        cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id2, timestamp, value, unit))
        conn.commit()
        conn.close()
