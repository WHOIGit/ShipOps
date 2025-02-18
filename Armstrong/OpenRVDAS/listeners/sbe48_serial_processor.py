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

def process_line(line, sock):
    """Process a single line of the log file and send to appropriate output."""
    line = line.rstrip()

    if line == "":
        output = f"SSW {get_timestamp()} SBE48\n"
    else:
        output = f"SSW {get_timestamp()} SBE48 {line}\n"

    print(f"DEBUG UDP OUTPUT: {output.strip()}")  # Debug statement to verify output format

    with open('SBE48.log', 'a') as f:
        f.write(output)
    sock.sendto(output.encode(), ('10.100.100.30', 57322))
        
def main():
    # Create output directory if it doesn't exist
    Path('SBE48.log').touch()
    
    # Setup UDP socket
    sock = setup_udp_socket()
    
    try:
        # Setup serial port
        ser = serial.Serial(
            port='/dev/ttyUSB22',
            baudrate=9600,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1  # 1 second timeout for reading
        )
        
        # Continuously read from serial port
        while True:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8')
                    process_line(line, sock)
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
