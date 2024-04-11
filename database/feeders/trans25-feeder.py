#!/usr/bin/env python
# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time
import math

while True: 
    # UDP message from datalog
    port = 55508
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.bind(("",port))
    
    # Receive and store udp msg
    data, addr = sock.recvfrom(1024)
    msg = data.decode()
    string = msg.split()
    #print (string)
	
    # Define variables
    VdarkS = 0
    Vdark = 0
    Vref = 15451 #from cal sheet
    VairS = 8183 #from ship cal
    Vair = 15809
    z = 0.25
    
    Vsig = float(string[6])
    id = "Trans25"    
    timestamp = int(time.time())
    unit = " Tr"
    
    # Transmittance Ratio (Tr) = [(Vsig - VdarkS)/(Vref -Vdark)] * [(Vair -Vdark)/(VairS - VdarkS)]
    Tr = ( ( Vsig - VdarkS ) / (Vref -Vdark) ) * ( ( Vair - Vdark ) / ( VairS - VdarkS ) )
    value = "%.2f" % Tr # Precision 2 decimal places

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