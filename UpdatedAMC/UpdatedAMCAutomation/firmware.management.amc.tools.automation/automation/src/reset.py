import time
import load_util
import configs  # Updated import
import serial_util
import sys

def uart_boot(ser):
    load_util.load_image(ser, "uart boot")

def warm(ser):
    load_util.load_image(ser, "reset warm")

def cold(ser):
    load_util.load_image(ser, "reset cold")

if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    try:
        warm(ser)
        time.sleep(1)
        cold(ser)
    except Exception as e:
        print(str(e))
        sys.exit(1)