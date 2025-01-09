import serial
import sys

def read_serial_data(port='/dev/ttyUSB14', baudrate=9600):
    try:
# Open the serial port
        ser = serial.Serial(port=port, baudrate=baudrate, timeout=1)
                                                            
# Continuously read and process data
        while True:
            try:
# Read a line of data
                 line = ser.readline()
                                                                                                                        # Decode the line and strip all whitespace
                 processed_line = line.decode('utf-8').strip()

# Only print non-empty lines
                 if processed_line:
                     print(processed_line)
                     sys.stdout.flush()  # Ensure immediate output
            except UnicodeDecodeError:
                                                                                                                        # Handle potential decoding errors
                print("Error: Could not decode received data", file=sys.stderr)

            except KeyboardInterrupt:
                                                                                                                        # Allow clean exit with Ctrl+C
                print("\nSerial reading stopped.", file=sys.stderr)
                break
    except serial.SerialException as e:
        print(f"Error opening serial port {port}: {e}", file=sys.stderr)
        sys.exit(1)
    
    finally:

# Ensure the serial port is closed
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    read_serial_data()
