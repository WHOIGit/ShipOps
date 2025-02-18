import socket
import datetime
import sys
from pathlib import Path

# Define the base log directory
LOG_DIR = Path('/home/admin_paul.mena/logs')

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

def get_log_filename(data_type):
    """Generate log filename with current date and hour.
    
    Args:
        data_type: String indicating the type of data ('XTP', 'XTS', or 'FLR')
    """
    now = datetime.datetime.now()
    date_str = now.strftime('%Y%m%d')
    hour_str = now.strftime('%H')
    return f"ar{date_str}_{hour_str}00.{data_type}"

def ensure_log_directory():
    """Create log directory if it doesn't exist."""
    LOG_DIR.mkdir(parents=True, exist_ok=True)

def get_log_path(data_type):
    """Get full path for log file."""
    filename = get_log_filename(data_type)
    return LOG_DIR / filename

def process_line(line, sock):
    """Process a single line of data and send to appropriate output."""
    line = line.strip()
    if not line:
        return

    timestamp = get_timestamp()
    
    if line.startswith('PR0'):
        output = f"MET {timestamp} WXTP {line}"
        with open(get_log_path('XTP'), 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57304))
        
    elif line.startswith('SR0'):
        output = f"MET {timestamp} WXTS {line}"
        with open(get_log_path('XTS'), 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57404))
        
    elif line.startswith('*+'):
        output = f"SSW {timestamp} FLR {line}"
        with open(get_log_path('FLR'), 'a') as f:
            f.write(output + '\n')
        sock.sendto(output.encode(), ('localhost', 57504))

def main():
    # Ensure log directory exists
    ensure_log_directory()
    
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
