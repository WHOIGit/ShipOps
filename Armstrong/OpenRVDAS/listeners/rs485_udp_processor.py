import socket
import datetime
import sys
from pathlib import Path

def setup_udp_socket(port):
    """Create and bind a UDP socket to receive data."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('localhost', port))
    return sock

def setup_output_socket():
    """Create a UDP socket for sending data."""
    return socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def get_timestamp():
    """Generate current timestamp in the required format."""
    return datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def process_line(line, sock):
    """Process a single line of data and send to appropriate output."""
    line = line.strip()
    if not line:
        return

    timestamp = get_timestamp()
    
    if line.startswith('PR0'):
        output = f"MET {timestamp} WXTP {line}"
        with open('port.log', 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57304))
        
    elif line.startswith('SR0'):
        output = f"MET {timestamp} WXTS {line}"
        with open('starboard.log', 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57404))
        
    elif line.startswith('*+'):
        output = f"SSW {timestamp} FLR {line}"
        with open('fluor.log', 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57504))

def main():
    # Create output directory if it doesn't exist
    Path('port.log').touch()
    Path('starboard.log').touch()
    Path('fluor.log').touch()
    
    # Setup UDP sockets
    input_sock = setup_udp_socket(57204)
    output_sock = setup_output_socket()
    
    try:
        while True:
            data, addr = input_sock.recvfrom(1024)
            line = data.decode().strip()
            process_line(line, output_sock)
                
    except Exception as e:
        print(f"Error processing UDP data: {e}")
        sys.exit(1)
    finally:
        input_sock.close()
        output_sock.close()

if __name__ == "__main__":
    main()
