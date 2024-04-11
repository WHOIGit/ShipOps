#!/usr/bin/env python
#script to make a new sqlite database 

import sqlite3
import time

# Path to database
dbfile = "armstrong.db"

# Define variables
jsVariable = "range"
id = "Range"
timestamp = int(time.time())
value = 5
unit = " nm"

# Connect to the database
conn = sqlite3.connect(dbfile)
cursor = conn.cursor()

# Insert the data into the table columns
cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id, timestamp, value, unit))

conn.commit()
conn.close()
