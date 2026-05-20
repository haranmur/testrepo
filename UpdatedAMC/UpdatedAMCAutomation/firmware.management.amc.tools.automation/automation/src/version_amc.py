import configs  # Updated import
import serial_util
import sys

def check_amc_version(ser):
    cmd="version amc"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

def run_all_tests(ser):
    import report_util
    try:
        check_amc_version(ser)
        report_util.add_result("amc version check", "Success", "amc version check commands successful")
    except Exception as e:
        report_util.add_result("amc version check", "Failure", str(e))
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
    print("All amc version tests completed.")
