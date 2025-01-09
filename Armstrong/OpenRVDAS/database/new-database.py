#!/usr/bin/env python3
#
# 2023 EFC
#
# Python script to make a new sqlite database 

import sqlite3
import json

dbfile = "/var/www/html/database/armstrong.db" # Set the database name and absolute filepath

# Initiate and connect to the database
conn = sqlite3.connect(dbfile)
cursor = conn.cursor()

table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='winch'; """).fetchall() # Query target table 
 
# If table does not exist, create one
if table == []:

    # Create and format a table named "array"
    print ("Generating table..")
    table = """ CREATE TABLE winch ( 
                timestamp TEXT PRIMARY KEY, 
                payout TEXT NOT NULL, 
                tension TEXT NOT NULL,
                speed TEXT NOT NULL
            ); """
        
    cursor.execute(table)
    print ("Table complete")
    
else: 
    print ("Table exists")
    conn.close()
