#!/usr/bin/python3
#script to check contents of the sqlite database 

import sqlite3
import json
import datetime
import time

dbfile = "/var/www/html/database/atlantis.db"

# Connect to the database
conn = sqlite3.connect(dbfile)
cursor = conn.cursor()

# Select all data from the table
cursor.execute('SELECT * FROM array')
result = cursor.fetchall()
conn.close()

# Iterate variables of each row in the database using for loop
for row in result:
    id = row[0]
    epoch = row[1]
    value = str(row[2])
    unit = row[3]
    
    # Convert time since epoch to date/time
    timestamp = str(datetime.datetime.fromtimestamp(epoch))
    
    # Calculate age of latest data in seconds
    sec = int(time.time()) - int(epoch)
    
    # Set flag for old/good data
    if (sec > 300): 
        flag = "OLD DATA"
    else: 
        flag = "GOOD"
    
    list = [timestamp, id, (value + unit), flag]
    
    # Print out readable database to CLI
    print ("{: <25} {:<18} {:<25} {:<20}".format(*list))
