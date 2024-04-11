#!/usr/bin/env python
# efc 2023 store UDP message in sqlite database

# To make a new feeder replace all instances of ###: 
    # 1. Add UDP port 
    # 2. Add "id" to match html "id" 
    # 3. Add numeric "value" from "string" 
    # 4. Add "unit"

import socket
import json
import time
import re

while True: 
    # UDP message from datalog
    port = ###
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg as string
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = msg.split()
    #string = re.split(r'=|,| ', msg)   # Use "re" module if splitting a message by 2 deliminators
    #print (string)
    
    # Define variables 
    timestamp = int(time.time())
    
    # Example:
    #id1 = "SSV"
    #value1 = string[2]
    #unit1 = "m/s"
    
    id1 = "###"  
    value1 = ###
    unit1 = "###"
    
    #id2 = "###"  
    #value2 = ###
    #unit2 = "###"
    
    #id3 = "###"  
    #value3 = ###
    #unit3 = "###"
    
    # Connect to sqlite db
    dbfile = "armstrong.db"
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()
    table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
    if table == []: # Check if table exists
        print ("Table does not exist")
        
    else: # Write to the sqlite database
                
        cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value1, unit1))
        
        #cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id2, timestamp, value2, unit2))
                  
        #cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id3, timestamp, value3, unit3))
        
        conn.commit()
        conn.close()