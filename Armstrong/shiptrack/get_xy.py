import csv
import re
import os
import subprocess
from datetime import datetime

def strip(s):
    """Helper function to strip leading and trailing whitespace"""
    return s.strip()

def extract_date(filename):
    """Helper function to extract the date from the filename"""
    match = re.search(r'(\d{6})', filename)
    if match:
        # Convert the date string (YYMMDD) to a datetime object for sorting
        return datetime.strptime(match.group(1), '%y%m%d')
    return None

if __name__ == '__main__':
    debug = True

    out_file = 'gmt.xy'
    out_dir = '/home/admin_paul.mena/shiptrack'
    data_dir = '/home/data/underway/proc'
    last_out = '{}/last.xy'.format(out_dir)
    label = '{}/gmt.label'.format(out_dir)
    minmax = '{}/minmax'.format(out_dir)

    cmd = subprocess.check_output(['cat', '/home/data/CRUISE_ID'])
    cruise_id = cmd.decode('utf-8').strip()
    ship_id = cruise_id[:2]

    if debug:
        print('out_file = {}'.format(out_file))
        print('out_dir = {}'.format(out_dir))
        print('data_dir = {}'.format(data_dir))
        print('last_out = {}'.format(last_out))
        print('label = {}'.format(label))
        print('minmax = {}'.format(minmax))
        print('cruise_id = {}'.format(cruise_id))
        print('ship_id = {}'.format(ship_id))

    lats, lons = [], []
    
    # Find all CSVs in the data directory
    csv_file_list = []
    for filename in os.listdir(data_dir):
        if re.search(r'{}(\d{{6}})_.*\.csv'.format(ship_id), filename, flags=re.IGNORECASE):
            csv_file_list.append(os.path.join(data_dir, filename))

    # Sort files by the extracted date
    csv_file_list.sort(key=lambda x: extract_date(os.path.basename(x)))

    if debug:
        print('csv_file_list (sorted) = {}'.format(csv_file_list))
        
    try:
        # Open the output file once outside the loop
        with open(os.path.join(out_dir, out_file), 'w') as writer:
            # Start reading CSVs
            for csv_file in csv_file_list:
                try:
                    with open(csv_file) as f:
                        reader = csv.DictReader(f)

                        # Discard title line
                        skipping = next(f)
                        if debug:
                            print('skipping title: {}'.format(skipping))

                        # Read content
                        for row in reader:
                            lat_str = row.get(' Dec_LAT', '')
                            lon_str = row.get(' Dec_LON', '')
                            date = row.get('DATE_GMT', '')
                            time = row.get(' TIME_GMT', '')

                            if lat_str and lon_str:  # check for non-empty strings
                                try:
                                    lat = float(lat_str)
                                    lon = float(lon_str)
                                    # Check for valid lat/lon
                                    writer.write('{}, {}, {}, {}\n'.format(lon, lat, time, date))
                                    lats.append(lat)
                                    lons.append(lon)
                                except ValueError:
                                    # Handle conversion error
                                    print('Invalid lat/lon data: {}, {}'.format(lat_str, lon_str))
                except csv.Error as e:
                    print("CSV error: {}".format(e))
                except IOError as e:
                    print('Error {}'.format(e))
                except Exception as e:
                    print("An unexpected error occurred: {}".format(e))

        if lats and lons:
            # Write min and max values
            lats = sorted(lats)
            lah = lats[-1]
            lal = lats[0]

            lons = sorted(lons)
            lol = lons[0]
            loh = lons[-1]

            with open(minmax, "w") as minmax_file:
                minmax_file.write("{}, {}, {}, {}\n".format(lal, lah, lol, loh))

            # Current time as placeholders for now and now_t
            now = datetime.now().strftime('%Y-%m-%d')
            now_t = datetime.now().strftime('%H:%M:%S')

            # Write last out values
            with open(last_out, "w") as last_out_file:
                last_out_file.write("{} {} {} {} \n".format(lons[-1], lats[-1], now, now_t))

            # Write label values
            with open(label, "w") as label_file:
                label_file.write("{} {} 16 0 6 1 {}\n".format(lons[-1], lats[-1], cruise_id))

                if lats[-1] < 0:
                    latc = abs(lats[-1])
                    NS = "S"
                else:
                    latc = lats[-1]
                    NS = "N"

                if lons[-1] < 0:
                    lonc = abs(lons[-1])
                    EW = "W"
                else:
                    lonc = lons[-1]
                    EW = "E"

                lastmsg = "Last position: {} {}, {} {} at {} gmt on {}\n".format(latc, NS, lonc, EW, now_t, now)
                label_file.write("{} {} 14 0 4 1 {}\n".format(lons[-1], lats[-1], lastmsg))
    except IOError as e:
        print("Error opening output file: {}".format(e))
