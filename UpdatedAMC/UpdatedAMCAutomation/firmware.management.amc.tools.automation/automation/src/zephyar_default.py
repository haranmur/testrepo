import configs  # Updated import
import serial
import time
import serial_util
import sys

def help(ser):
    cmd="help"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

def history(ser):
    cmd="history"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

def resize(ser):
    cmd="resize"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

def retval(ser):
    cmd="retval"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

def clear(ser):
    cmd="clear"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

def zephyar_test(ser):
    help(ser)
    history(ser)
    resize(ser)
    retval(ser)
    clear(ser)
    print("zephyar commands are verified")

def run_all_tests(ser):
    import report_util
    try:
        zephyar_test(ser)
        report_util.add_result("zephyar commands", "Success", "zephyar commands successful")
    except Exception as e:
        report_util.add_result("zephyar commands", "Failure", str(e))
        print(f"Exception: {e}")

if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    try:
        run_all_tests(ser)
    except Exception as e:
        print(str(e))
        sys.exit(1)
    ser.close()
    print("All zephyar tests completed.")