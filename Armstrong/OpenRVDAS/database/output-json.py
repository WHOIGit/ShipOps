#!/usr/bin/env python3
#
# 2024 EFC
#
# Python script that queries the sqlite database and outputs contents in valid JSON format
#
# To be run continuously in the background as user: sssg

import sqlite3
import datetime
import time
import json

dbfile = "/var/www/html/database/atlantis.db" # absolute path to target sqlite database

while True: 

    # Connect to the database
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()

    # Select all data from the table "array"
    cursor.execute('SELECT * FROM array')
    result = cursor.fetchall()
    conn.close()

    # Establish a dictionary
    data = {}

    # Iterate through each row in the database using for loop
    for row in result:
        
        id = row[0]
        timestamp = row[1]
        value = str(row[2])
        unit = row[3]
    
        # Establish format and content of nested data
        newRow = {"timestamp": timestamp, "id": id, "value": value, "unit": unit} 
    
        data[id] = newRow # Set "id" as the parent of the nested row into the data dictionary
    
    # Output dictionary to file as json
    with open("data.json", "w") as file:
        json.dump(data, file)
    
    time.sleep(0.5) # Repeat the script every 0.5 seconds
