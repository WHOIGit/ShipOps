#!/usr/bin/env python3
"""
Grabs information out of datascreen.db and creates true wind for 
both port and starboard vaisala.

Converted from Perl to Python 3
Original: lstolp 201302ish
"""

import sys
import time
import math
import sqlite3
import argparse
from datetime import datetime


class AtlWindCalculator:
    def __init__(self, debug=False, debughash=False):
        self.debug = debug
        self.debughash = debughash
        
        # SQLite parameters
        self.dsdb = "atlantis.db"
        self.db_name = f"/var/www/html/database/{self.dsdb}"
        
        # Wind parameters
        self.cog = None          # course over ground
        self.mcog = None         # corrected cog
        self.sog_inknots = None  # speed over ground
        self.hdt = None          # heading from gyro
        self.zlr = 0             # Zero line reference -- angle between bow and
                                # zero line on anemometer. Direction is clockwise
                                # from the bow. (Use bow=0 degrees as default 
                                # when reference not known.)
        self.wdir = None         # wind direction measured by anemometer
        self.wspd = None         # wind speed measured by anemometer
        self.wind_sensors = ["port", "stbd"]
        
        # Additional variables
        self.svalue = None
        self.dvalue = None
        self.mtdir = None        # correction for angle of true wind
        
        # Conversion factors
        self.dtor = math.pi / 180  # degrees to radians conversion
        
    def connect_database(self):
        """Connect to SQLite database"""
        try:
            conn = sqlite3.connect(self.db_name)
            if self.debug:
                print(f"\nStart of while loop Database connected: {conn}", file=sys.stderr)
            return conn
        except sqlite3.Error as e:
            print(f"Cannot connect to database: {e}", file=sys.stderr)
            sys.exit(1)
    
    def get_data_from_db(self):
        """Retrieve data from database and return as dictionary"""
        conn = self.connect_database()
        
        # Time variable to test how old the data in the db is
        current_time = time.strftime("%H:%M:%S", time.localtime())
        epoch_time = int(time.time())
        
        try:
            cursor = conn.cursor()
            cursor.execute("SELECT id, value, timestamp FROM array")
            rows = cursor.fetchall()
        except sqlite3.Error as e:
            print(f"Database query error: {e}", file=sys.stderr)
            conn.close()
            return {}
        
        # Disconnect after reading data (as in original Perl code)
        conn.close()
        
        data = {}
        
        # Process each row from database
        for row in rows:
            key, value, time_enter = row
            lapsetime = epoch_time - time_enter
            
            if self.debug:
                print(f"The key is {key}")
                print(f"value = {value}")
                print(f"epoch_time = {epoch_time}")
                print(f"timestamp = {time_enter}")
                print(f"lapsetime = {lapsetime}\n")
            
            # If greater than 45 seconds old, use nan
            if lapsetime > 45:
                value = "nan"
            
            data[key] = value
        
        # Debug: print hash contents
        if self.debughash:
            for key, value in data.items():
                print(f"{key} => {value}")
        
        return data
    
    def break_wind(self):
        """Calculate true wind speed and direction"""
        
        # Convert SOG from knots to meters per second (1 Knot = 0.514 m/s)
        sog = self.sog_inknots * 0.514
        
        # Convert from navigational coordinates to angles commonly used in mathematics
        self.mcog = 90.0 - self.cog
        
        # Keep the value between 0 and 360 degrees
        if self.mcog <= 0.0:
            self.mcog += 360.0
        
        # Check zlr for valid value. If not valid, set equal to zero.
        if self.zlr < 0.0 or self.zlr > 360.0:
            self.zlr = 0.0
        
        # Calculate apparent wind direction
        adir = self.hdt + self.wdir + self.zlr
        
        # Keep adir between 0 and 360 degrees
        if adir >= 360.0:
            adir -= 360.0
        
        # Convert from meteorological coordinates to angles commonly used in mathematics
        mwdir = 270.0 - adir
        
        # Keep the value between 0 and 360 degrees
        if mwdir <= 0.0:
            mwdir += 360.0
        elif mwdir > 360.0:
            mwdir -= 360.0
        
        # Determine the East-West and North-South vector components of the true wind
        x = (self.wspd * math.cos(mwdir * self.dtor)) + (sog * math.cos(self.mcog * self.dtor))
        y = (self.wspd * math.sin(mwdir * self.dtor)) + (sog * math.sin(self.mcog * self.dtor))
        
        # Use the two vector components to calculate the true wind speed
        tspd = math.sqrt((x * x) + (y * y))
        calm_flag = 1
        
        # Determine the angle for the true wind
        if abs(x) > 0.00001:
            self.mtdir = math.atan2(y, x) / self.dtor
        else:
            if abs(y) > 0.00001:
                self.mtdir = 180.0 - (90.0 * y) / abs(y)
            else:
                # The true wind speed is essentially zero: winds are calm
                # and direction is not well defined
                self.mtdir = 270.0
                calm_flag = 0
        
        # Convert from the common mathematical angle coordinate to the meteorological wind direction
        tdir = 270.0 - self.mtdir
        
        # Make sure that the true wind angle is between 0 and 360 degrees
        if tdir < 0.0:
            tdir = (tdir + 360.0) * calm_flag
        if tdir > 360.0:
            tdir = (tdir - 360.0) * calm_flag
        
        # Get current time and format it like the original Perl code
        ltime = time.localtime()
        formatted_time = time.strftime("%H:%M:%S", ltime)
        
        # Format to 2 decimal places
        tspd = round(tspd, 2)
        tdir = round(tdir, 2)
        
        if self.debug:
            print(f"{formatted_time} tspd = {tspd}")
            print(f"{formatted_time} tdir = {tdir}")
        
        # Store wind data into the database
        self.store_wind_data(formatted_time, tspd, tdir)
    
    def store_wind_data(self, formatted_time, tspd, tdir):
        """Store calculated wind data back to database"""
        conn = self.connect_database()
        
        try:
            cursor = conn.cursor()
            
            # Insert or replace wind speed
            cursor.execute(
                "INSERT OR REPLACE INTO array (id, timestamp, value) VALUES (?, ?, ?)",
                (self.svalue, formatted_time, str(tspd))
            )
            
            # Insert or replace wind direction
            cursor.execute(
                "INSERT OR REPLACE INTO array (id, timestamp, value) VALUES (?, ?, ?)",
                (self.dvalue, formatted_time, str(tdir))
            )
            
            # Set pragma for performance
            cursor.execute("PRAGMA synchronous = OFF")
            conn.commit()
            
        except sqlite3.Error as e:
            print(f"Database insert error: {e}", file=sys.stderr)
        finally:
            conn.close()
    
    def process_wind_data(self, data):
        """Process wind data for each sensor"""
        
        # Get basic navigation data
        try:
            self.sog_inknots = float(data.get('SOG', 0))
            self.cog = float(data.get('COG', 0))
            self.hdt = float(data.get('GYRO', 0))
        except (ValueError, TypeError):
            print("Error: Invalid navigation data", file=sys.stderr)
            return
        
        # Process each wind sensor
        for wind in self.wind_sensors:
            try:
                if wind == "port":
                    if self.debug:
                        print("Using Port Vaisala")
                    
                    self.wdir = float(data.get('WXTP_Dm', 0))
                    self.wspd = float(data.get('WXTP_Sm', 0))
                    self.dvalue = "WXTP_TD"
                    self.svalue = "WXTP_TS"
                    
                    if self.debug:
                        print(f"wdir = {data.get('WXTP_Dm')}")
                        print(f"wspd = {data.get('WXTP_Sm')}")
                    
                    self.break_wind()
                
                elif wind == "stbd":
                    if self.debug:
                        print("\nUsing STBD Vaisala")
                    
                    self.wdir = float(data.get('WXTS_Dm', 0))
                    self.wspd = float(data.get('WXTS_Sm', 0))
                    self.dvalue = "WXTS_TD"
                    self.svalue = "WXTS_TS"
                    
                    if self.debug:
                        print(f"wdir = {data.get('WXTS_Dm')}")
                        print(f"wspd = {data.get('WXTS_Sm')}")
                    
                    self.break_wind()
                    
            except (ValueError, TypeError) as e:
                print(f"Error processing {wind} wind data: {e}", file=sys.stderr)
                continue
    
    def run(self):
        """Main execution loop"""
        if self.debug:
            print("Starting ATL wind calculation program", file=sys.stderr)
        
        while True:
            try:
                # Get data from database
                data = self.get_data_from_db()
                
                if not data:
                    print("No data retrieved from database", file=sys.stderr)
                    time.sleep(1)
                    continue
                
                # Process wind calculations
                self.process_wind_data(data)
                
                # Sleep for 1 second before next iteration
                time.sleep(1)
                
            except KeyboardInterrupt:
                print("\nProgram interrupted by user", file=sys.stderr)
                break
            except Exception as e:
                print(f"Unexpected error: {e}", file=sys.stderr)
                time.sleep(1)
                continue


def main():
    """Main function to parse arguments and run the program"""
    parser = argparse.ArgumentParser(description="ATL True Wind Speed and Direction Calculator")
    parser.add_argument('--debug', action='store_true', help='Enable debug output')
    parser.add_argument('--debughash', action='store_true', help='Enable hash debug output')
    
    args = parser.parse_args()
    
    # Create and run wind calculator
    calculator = AtlWindCalculator(debug=args.debug, debughash=args.debughash)
    calculator.run()


if __name__ == "__main__":
    main()
