import socket
import datetime
import sys
from pathlib import Path

def setup_udp_socket():
    """Create and return a UDP socket."""
    return socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def get_timestamp():
    """Generate current timestamp in the required format."""
    return datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def process_line(line, sock):
    """Process a single line of the log file and send to appropriate output."""
    line = line.strip()
    if not line:
        return

    timestamp = get_timestamp()
    
    if line.startswith('PR0'):
        # Process port side weather data
        output = f"MET {timestamp} WXTP {line}"
        with open('port.log', 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57304))
        
    elif line.startswith('SR0'):
        # Process starboard side weather data
        output = f"MET {timestamp} WXTS {line}"
        with open('starboard.log', 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57404))
        
    elif line.startswith('*+'):
        # Process fluorometer data
        output = f"SSW {timestamp} FLR {line}"
        with open('fluor.log', 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57504))

def main():
    # Create output directory if it doesn't exist
    Path('port.log').touch()
    Path('starboard.log').touch()
    Path('fluor.log').touch()
    
    # Setup UDP socket
    sock = setup_udp_socket()
    
    try:
        # Process the input file line by line
        with open('serial_485_stripped.log', 'r') as f:
            for line in f:
                process_line(line, sock)
                
    except FileNotFoundError:
        print("Error: Input file 'serial_485_stripped.log' not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error processing file: {e}")
        sys.exit(1)
    finally:
        sock.close()

if __name__ == "__main__":
    main()
