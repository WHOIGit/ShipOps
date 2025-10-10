import socket
import datetime
import sys
import serial
from pathlib import Path

def setup_udp_socket():
    """Create and return a UDP socket."""
    return socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def get_timestamp():
    """Generate current timestamp in the required format."""
    return datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def get_log_filename():
    """Generate log filename in the format at + YYYYMMDD_HH + 00.SBE48"""
    now = datetime.datetime.now()
    return f"/home/sssg/openrvdas_support/logs/at{now.strftime('%Y%m%d_%H')}00.SBE48"

def get_current_hour():
    """Get the current hour for tracking hour changes."""
    return datetime.datetime.now().hour

def process_line(line, sock, current_log_file):
    """Process a single line of the log file and send to appropriate output."""
    line = line.rstrip()

    if line == "":
        output = f"SSW {get_timestamp()} SBE48\n"
    else:
        output = f"SSW {get_timestamp()} SBE48 {line}\n"

#   print(f"DEBUG UDP OUTPUT: {output.strip()}")  # Debug statement to verify output format

    with open(current_log_file, 'a') as f:
        f.write(output)
    sock.sendto(output.encode(), ('10.100.161.10', 57302))
        
def main():
    # Create output directory if it doesn't exist
    log_dir = Path('/home/sssg/openrvdas_support/logs')
    log_dir.mkdir(parents=True, exist_ok=True)
    
    # Setup UDP socket
    sock = setup_udp_socket()
    
    # Initialize log file tracking
    current_hour = get_current_hour()
    current_log_file = get_log_filename()
    print(f"Starting logging to: {current_log_file}")
    
    try:
        # Setup serial port
        ser = serial.Serial(
            port='/dev/ttyUSB2',
            baudrate=9600,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1  # 1 second timeout for reading
        )
        
        # Continuously read from serial port
        while True:
            try:
                # Check if hour has changed and update log file if needed
                new_hour = get_current_hour()
                if new_hour != current_hour:
                    current_hour = new_hour
                    current_log_file = get_log_filename()
                    print(f"Hour changed - switching to new log file: {current_log_file}")
                
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8')
                    process_line(line, sock, current_log_file)
            except serial.SerialException as e:
                print(f"Serial port error: {e}")
                break
            except UnicodeDecodeError as e:
                print(f"Error decoding serial data: {e}")
                continue
                
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error processing serial data: {e}")
        sys.exit(1)
    finally:
        sock.close()
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()