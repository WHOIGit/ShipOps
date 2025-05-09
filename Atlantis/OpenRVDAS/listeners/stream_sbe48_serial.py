import argparse
import multiprocessing
import os
import serial
import socket
import subprocess
import sys

################################

OPENRVDAS_PATH='/opt/openrvdas'
IN_PORT='/dev/ttyUSB22'
IN_BAUD=9600
BUFFER_PORT=56322
INTERVAL=8
TIME_FORMAT='%Y/%m/%d %H:%M:%S.%f'
LABEL1='SBE48'
LABEL2='SSW'
LOG_DIR='/home/admin_paul.mena/logs'
OUT_PORT=57322
OUT_HOST='10.100.100.30'

################################

def read_serial_data():
    try:
        # Open network connections
        ser = serial.Serial(port=IN_PORT, baudrate=IN_BAUD, timeout=1)
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # Continuously read and process data
        while True:
            try:
                # Read and send non blank lines of data
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    sock.sendto(line.encode(), ('127.0.0.1', BUFFER_PORT))

            except UnicodeDecodeError as e:
                print(f'Error decoding received data: {e}', file=sys.stderr)

            except KeyboardInterrupt:
                print('\nSerial reading stopped.', file=sys.stderr)
                break

    except serial.SerialException as e:
        print(f'Error opening serial port {IN_PORT}: {e}', file=sys.stderr)
        sys.exit(1)

    finally:
        # Ensure the serial port is closed
        if 'ser' in locals():
            ser.close()

################################

def run_openrvdas_listener():
    # Run listener
    venv_exec = os.path.join(OPENRVDAS_PATH, 'venv/bin/python')
    subprocess.Popen([
        venv_exec,
        'logger/listener/listen.py',
        '--udp', str(BUFFER_PORT),
        '--interval', str(INTERVAL),
        '--transform_prefix', LABEL1,
        '--time_format', TIME_FORMAT,
        '--transform_timestamp',
        '--transform_prefix', LABEL2,
        '--write_file', f'{LOG_DIR}/serial_SBE48.log',
        '--write_udp', f'{OUT_HOST}:{OUT_PORT}'
    ], cwd=OPENRVDAS_PATH)

################################

if __name__ == '__main__':
    # Invoke both processes at the same time
    serial_proc = multiprocessing.Process(target=read_serial_data)
    listener_proc = multiprocessing.Process(target=run_openrvdas_listener)

    try:
        serial_proc.start()
        listener_proc.start()
        serial_proc.join()
        listener_proc.join()

    except KeyboardInterrupt:
        print('\nStopping both processes')
        serial_proc.terminate()
        listener_proc.terminate()
        serial_proc.join()
        listener_proc.join()
        sys.exit(0)
