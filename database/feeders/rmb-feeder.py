#!/usr/bin/env python

# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time

while True: 
    # UDP message from openCPN server
    port = 55055
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = msg.split(",")
    #print (string)

    if string [0] == "$ECRMB":
        if float(string[12]) > 0 : #if speed is greater than 0 kts
    
            # Define variables 
            timestamp = int(time.time())
            
            id1 = "Range"  
            value1 = string[10]
            unit1 = " nm"
    
            id2 = "ETA"
            hours = float(string[10]) / float(string[12])
            hour = int(hours)
            minutes = abs(int( 60 * ( hours - hour )))
            
            # Conditional formatting when on station
            if  float(string[12]) < 0.2 :
                hour = "00"
                minute = "00"

            elif float(string[12]) > 0.2 :

                if minutes < 10 :
                    minute = "0" + str(minutes)
                                          

                elif minutes > 10: 
                    minute = minutes 

                         
                    eta = str(hour) + ":" + str(minute) 
                    value2 = eta
                    unit2 = ""
                    
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
                
