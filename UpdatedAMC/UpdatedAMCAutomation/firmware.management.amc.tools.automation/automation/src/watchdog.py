import serial_util
import sys
import time
import load_util
import configs

def feed_stop(ser):
    load_util.load_image(ser, "watchdog feed_stop")

if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    feed_stop(ser)
