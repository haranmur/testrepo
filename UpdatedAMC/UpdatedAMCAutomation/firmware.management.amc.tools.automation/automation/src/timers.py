import configs  # Updated import
import serial_util
import sys

def scan_timer(ser):
    cmd = "timer scan_timer 2"
    output = serial_util.send_command_and_read(ser, cmd, timeout=2)
    print(f"output: {output}")
    if ("Sensor_scan_timer_tick before user_delay of 2 sec" in output):
        print("✅ Success:scan timer successful")
    else:
        raise Exception(
            "❌ Failure: timer Failed! Alert !!! the timer scan failed")


def timer_scan_reinit(ser):
    cmd = "timer scan_timer_reinit"
    output = serial_util.send_command_and_read(ser, cmd, timeout=2)
    print(f"output: {output}")
    if ("Scan Timer reinit Completed" in output):
        print("✅ Success:timer scan reinit successful")
    else:
        raise Exception(
            "❌ Failure: timer Failed! Alert !!! the timer reinit scan failed")


def timer_scan_timer_stop(ser):
    cmd = "timer scan_timer_stop 2"
    output = serial_util.send_command_and_read(ser, cmd, timeout=2)
    print(f"output: {output}")
    if ("Sensor scan timer stopped" in output):
        print("✅ Success:scan timer stop successful")
    else:
        raise Exception(
            "❌ Failure: timer Failed! Alert !!! the scan timer stop failed")


def timer_hr(ser):
    cmd = "timer hr_timer 2"
    output = serial_util.send_command_and_read(ser, cmd, timeout=2)
    print(f"output: {output}")
    if ("HR timer ticks in ms is 2000" in output):
        print("✅ Success:HR timer ticks successful")
    else:
        raise Exception(
            "❌ Failure: timer Failed! Alert !!! HR timer ticks failed")


def wallclock_timer(ser):
    cmd = "timer wallclock_timer 2"
    output = serial_util.send_command_and_read(ser, cmd, timeout=2)
    print(f"output: {output}")
    if ("Wallclock timer functionality is verified" in output):
        print("✅ Success:Wallclock timer successful")
    else:
        raise Exception(
            "❌ Failure: timer Failed! Alert !!! Wallclock timer failed")


def timer_test(ser):
    scan_timer(ser)
    timer_scan_timer_stop(ser)
    timer_scan_reinit(ser)
    scan_timer(ser)
    timer_hr(ser)
    wallclock_timer(ser)
    print("timer tests are completed")


def run_all_tests(ser):
    import report_util
    try:
        timer_test(ser)
        report_util.add_result("timer test", "Success",
                               "timer test successful")
    except Exception as e:
        report_util.add_result("timer test", "Failure", str(e))
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
    print("All timer tests completed.")
