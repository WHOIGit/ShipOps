#!/usr/bin/env python
#
# 2024 EFC
#
# Python script that queries the sqlite database and outputs winch data
#

import os
import sqlite3
import time
import csv
from csv import writer

dbfile = "/var/www/html/database/armstrong.db"
directory = "/var/www/html/dygraphs/csv/"
filename1 = "winch24.csv"
filename2 = "winch1.csv"
header = ['Datetime', 'Tension', 'Payout', 'Speed']

while True: 
    # Connect to the database
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()

    # Select specified rows from the table "winch"
    cursor.execute("SELECT * FROM winch")
    result24 = cursor.fetchall()
    
    cursor.execute("SELECT * FROM winch ORDER BY timestamp DESC LIMIT 3600") # Limit to 1 hr of data
    result1 = cursor.fetchall()
    conn.close()

    filepath1 = os.path.join(directory, filename1) 
   
    with open(filepath1, 'w') as outfile:
                
        writer = csv.writer(outfile)   
        writer.writerow(header)
        writer.writerows(result24)
        outfile.close()

    filepath2 = os.path.join(directory, filename2) 
   
    with open(filepath2, 'w') as outfile:
                
        writer = csv.writer(outfile)   
        writer.writerow(header)
        writer.writerows(result1)
        outfile.close()

    time.sleep(1)
