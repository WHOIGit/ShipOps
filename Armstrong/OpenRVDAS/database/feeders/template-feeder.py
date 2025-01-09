#!/usr/bin/env python3
#
# UDP message to SQLite database
#    
########################################################

# To make a new feeder: 
    # 1. Define UDP port 
    # 2. Define "id" to match html "id" 
    # 3. Link numeric value from "string" 
    # 4. Define "unit"
    # 5. Path to database

import socket
import re
import time
import sqlite3

#######################################################

while True: 

    # Bind to UDP port
    
    port = ###          # 1. Change this to the target UDP port number 
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Parse the UDP message 
    
    data, addr = sock.recvfrom(1024)
    msg = data.decode() # decode the data to a string
    array = msg.split() # split message into an array based on delimiter 
    
    #array = re.split(r'=|,| ', msg) # Use the re.split method if the message needs to be split by multiple delimiters
    #print (array) # Uncomment this line to view/troubleshoot the array in CLI
    
    
########################################################
    
    # Define variables 
    
    timestamp = int(time.time()) # Make timestamp
    
    # EXAMPLE:
    #
    # id1 = "SSV"
    # value1 = array[2]
    # unit1 = "m/s"
    
    id1 = "###"         # 2. Define variable "id" to match html "id" attribute
    value1 = ###        # 3. Link numeric value from "array"
    unit1 = "###"       # 4. Define the unit of the variable 
    
    #id2 = "###"        # Option for multiple variables: also need to uncomment the corresponding cursor.execute below
    #value2 = ###
    #unit2 = "###"
  
########################################################

    # Connect and push variables to SQLite db
    
    dbfile = "/var/www/html/database/armstrong.db" # 5. Path to SQLite database
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()
    table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
    if table == []: # Check if the table exists
        print ("Table does not exist, please run make-database.py")
        conn.close()
        
    else: # Write latest data to the sqlite database
                
        cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id1, timestamp, value1, unit1))
        
        #cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id2, timestamp, value2, unit2))
        
        conn.commit()
        conn.close()