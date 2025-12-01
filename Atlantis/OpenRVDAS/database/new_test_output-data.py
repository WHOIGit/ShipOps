#!/usr/bin/python3
#script to make a new sqlite database 

import sqlite3
import json
import time

dbfile = "atlantis_test.db"

while True:
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()

    # Fetch all rows from the array table
    cursor.execute("SELECT id, timestamp, value, unit FROM array")
    
    # Result is a list of tuples
    rows = cursor.fetchall()
    
    # Convert to the object structure your HTML expects
    data = {}
    for row in rows:
        data[row[0]] = {
            "timestamp": row[1],
            "value": row[2],
            "unit": row[3]
        }
    
    # Write data to test_data.json file as properly formatted JSON
    with open("test_data.json", "w") as file:
        json.dump(data, file, indent=2)
    
    conn.close()
    time.sleep(0.5)
