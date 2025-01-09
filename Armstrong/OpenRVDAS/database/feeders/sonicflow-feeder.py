#!/usr/bin/env python3
# efc 2023 store UDP message in sqlite database

import socket
import json
import sqlite3
import time
import numbers
import logging

# Configure logging
logging.basicConfig(filename='/var/www/html/database/feeders/sonicflow_debug.log', level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

while True:
    try:
        # UDP message from datalog
        port = 55666
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("", port))

        # Receive and store udp msg
        data, addr = sock.recvfrom(1024)
        msg = data.decode()
        logging.debug(f"Received message: {msg}")

        string = msg.split()
        logging.debug(f"Split message: {string}")

        # Check if the message matches the expected format
        if len(string) < 5:
            logging.warning("Received message does not match expected format. Ignoring...")
            continue

        # Define variables
        id = "SonicFlow"
        timestamp = int(time.time())
        value = string[4]
        unit = " L/m"

        logging.debug(f"Variables - id: {id}, timestamp: {timestamp}, value: {value}, unit: {unit}")

        # Connect to sqlite db
        dbfile = "/var/www/html/database/armstrong.db"
        conn = sqlite3.connect(dbfile)
        cursor = conn.cursor()
        table = cursor.execute("""SELECT name FROM sqlite_master WHERE type='table' AND name='array'; """).fetchall()

        if table == []:  # Check if table exists
            logging.warning("Table does not exist")
        else:  # Write to the sqlite database
            cursor.execute("REPLACE INTO array (id, timestamp, value, unit) VALUES (?, ?, ?, ?)", (id, timestamp, value, unit))
            conn.commit()
            logging.info("Data written to database")
            conn.close()
    except Exception as e:
        logging.error("An error occurred:", exc_info=True)
    else:
        time.sleep(1)

