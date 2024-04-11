#!/usr/bin/env python
#script to make a new sqlite database 

import sqlite3
import time

dbfile = "armstrong.db"

while True:
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()

    # Outputs each row as a JSON object wrapped in a JSON array
    cursor.execute("SELECT json_group_array(json_object('id', id, 'timestamp', timestamp, 'value', value, 'unit', unit)) FROM array")

    # Result is a list of python tuples 
    result = cursor.fetchall()

    # Strip brackets from result
    data = str(result)[3:-4]

    # Write data to data.json file
    with open("data.json", "w") as file:
        file.write(data)

    time.sleep(0.5)
