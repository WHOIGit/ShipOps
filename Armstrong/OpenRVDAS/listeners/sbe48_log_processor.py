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
    line = line.rstrip()

    if line == "":
        output = f"SSW {get_timestamp()} SBE48\n"
    else:
        output = f"SSW {get_timestamp()} SBE48 {line}\n"

    print(f"DEBUG UDP OUTPUT: {output.strip()}")  # Debug statement to verify output format

    with open('SBE48.log', 'a') as f:
        f.write(output)
    sock.sendto(output.encode(), ('localhost', 57322))
        
def main():
    # Create output directory if it doesn't exist
    Path('SBE48.log').touch()
    
    # Setup UDP socket
    sock = setup_udp_socket()
    
    try:
        # Process the input file line by line
        with open('ar20250212_stripped.log', 'r') as f:
            for line in f:
                process_line(line, sock)
                
    except FileNotFoundError:
        print("Error: Input file 'ar20250212_stripped.log' not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error processing file: {e}")
        sys.exit(1)
    finally:
        sock.close()

if __name__ == "__main__":
    main()
