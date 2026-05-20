import configs  # Updated import
import serial_util
import sys


def monitor_mode(ser):
    cmd = "sensor monitor_mode 0"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)


def list_numeric(ser):
    monitor_mode(ser)
    cmd = "sensor list_numeric"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    if "BAD" in output:
        raise Exception(
            "❌ Failure: sensor list_numeric Verification Failed! Alert !!! the sensor list_numeric value has crossed the maximum value")
    else:
        print("✅ Success: sensor list_numeric Verification Passed")


def run_all_tests(ser):
    import report_util
    try:
        list_numeric(ser)
        report_util.add_result(
            "Sensor List Numeric", "Success", "Sensor list numeric verification passed")
    except Exception as e:
        report_util.add_result("Sensor List Numeric", "Failure", str(e))
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
    print("All sensor tests completed.")
