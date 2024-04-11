#!/usr/bin/env python
#script to make a new sqlite database 

import sqlite3
import json

dbfile = "armstrong.db"

# Connect to the database
conn = sqlite3.connect(dbfile)
cursor = conn.cursor()

table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()
 
# If table does not exist, create one
if table == []:

    # Creating table "array"
    print ("Generating table..")
    table = """ CREATE TABLE array ( 
                id TEXT NOT NULL PRIMARY KEY, 
                timestamp NUMBERIC, 
                value TEXT NOT NULL, 
                unit TEXT NOT NULL
            ); """
        
    cursor.execute(table)
    print ("Table complete")
    
else: 
    print ("Table exists")
    conn.close()
