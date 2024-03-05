import serial
import sys
import time

def send_data(serial_port, baud_rate, block_size):
    with serial.Serial(serial_port, baud_rate) as ser:
        while True:
            data = sys.stdin.buffer.read(block_size)
            if not data:
                break
            ser.write(data)
        ser.flush()
        time.sleep(1)
        ser.flush()  # Flush the serial port to ensure all data is sent
        print("Data sent successfully.")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python send_serial.py <serial_port> <baud_rate> <block_size>")
        sys.exit(1)

    serial_port = sys.argv[1]
    baud_rate = int(sys.argv[2])
    block_size = int(sys.argv[3])

    try:
        send_data(serial_port, baud_rate, block_size)
    except Exception as e:
        print("An error occurred:", e)
