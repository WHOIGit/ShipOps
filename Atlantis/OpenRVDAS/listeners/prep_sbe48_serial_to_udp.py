import serial
import socket

def forward_serial_to_udp(serial_port, udp_port, baud_rate=9600):

# Open the serial port
    ser = serial.Serial(serial_port, baud_rate, timeout=1)
                                                            
# Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                                                                
# Continuously read and process data
    try:
        while True:
# Read a line of data
             data = ser.readline()
                                                                                                                        # Decode the line and strip all whitespace
             data = data.strip()

# Only print non-empty lines
             if data:
                 sock.sendto(data, ('127.0.0.1', udp_port))

    except KeyboardInterrupt:
        print("Stopping serial to UDP forwarding")

    finally:

# Ensure the serial port is closed
            ser.close()
            sock.close()

if __name__ == "__main__":
    forward_serial_to_udp('/dev/ttyUSB14', 56314)
