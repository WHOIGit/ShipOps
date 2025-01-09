#!/usr/bin/env python3

import socket
import sqlite3
import time
import re

while True: 
    # UDP message from datalog
    port = 55401
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    array = re.split(r'=|,| ', msg)
         
    # Define Variables
    timestamp = int(time.time())

    id1 = "Humidity" 
    value1 = array[16][:-1]
    unit1 = "%"
    
    id2 = "Pressure" 
    value2 = array[18][:-1]
    unit2 = " hPa"
    
    id3 = "Temperature"     
    value3 = array[14][:-1]
    unit3 = "°C"

    # Connect to sqlite db
    dbfile = "/var/www/html/database/armstrong.db"
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
