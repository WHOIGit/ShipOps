#!/usr/bin/env python
#
# 2024 EFC
#
# Python script that queries the sqlite database and outputs posmv IMU data
#

import os
import sqlite3
import datetime
import time
import csv
from csv import writer

dbfile = "/var/www/html/database/armstrong.db" # absolute path to target sqlite database

while True: 
    # Connect to the database
    conn = sqlite3.connect(dbfile)
    cursor = conn.cursor()

    # Select specified rows from the table "array"
    cursor.execute("SELECT * FROM array WHERE id IN ( 'Roll', 'Pitch', 'Heave' )")
    result = cursor.fetchall()
    conn.close()

    header = ['Datetime', 'Roll', 'Pitch', 'Heave'] # Header for winch.csv
    directory = "/var/www/html/dygraphs/csv/" # Path to output csv directory
    filename = "live-rph.csv" # Output file name
    
    heave = result[0]
    pitch = result[1]
    roll = result[2]

    if (roll[1] == pitch[1]) : # Ensure the data for the variables are from the same UDP message
    
        timestamp = str(datetime.datetime.fromtimestamp(roll[1])) # Convert time since epoch in ms to date time string
        newData = [timestamp, roll[2], pitch[2], heave[2]] # Establish the new row of data

        file_path = os.path.join(directory, filename) 
  
        with open(file_path) as infile:
            content = csv.reader(infile)
            
            array = []
            for row in content:

                array.append(row)
        
            infile.close()
            
        if len(array) < 3000 : # limit file to 10 minutes
            array.append(newData)

            with open(file_path, 'w', newline='') as outfile:
                writer_object = writer(outfile)          
                writer_object.writerows(array)

                outfile.close()
                time.sleep(0.2)
    
        else:
        
            oldData = (array[2:len(array)]) #strip header and oldest data line
            oldData.append(newData) # append new data at end of csv
        
            with open(file_path, 'w') as outfile:
                
                writer = csv.writer(outfile)     
                writer.writerow(header)        
                writer.writerows(oldData)

                outfile.close()

            time.sleep(0.2)