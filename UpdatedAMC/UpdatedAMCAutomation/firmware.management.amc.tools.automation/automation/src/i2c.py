import configs  # Updated import
import serial_util
import sys


def i2c_scan0(ser):
    cmd = "i2c scan 0"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if "1 devices found on bus 0" in output:
        print("✅ Success: i2c scan0 test Passed")
    else:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c scan0 test failed")


def i2c_scan1(ser):
    cmd = "i2c scan 1"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if "1 devices found on bus 1" in output:
        print("✅ Success: i2c scan1 test Passed")
    else:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c scan1 test failed")


def i2c_scan3(ser):
    cmd = "i2c scan 3"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if "2 devices found on bus 3" in output:
        print("✅ Success: i2c scan3 test Passed")
    else:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c scan3 test failed")


def i2c_scan4(ser):
    cmd = "i2c scan 4"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if "2 devices found on bus 4" in output:
        print("✅ Success: i2c scan4 test Passed")
    else:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c scan4 test failed")


def i2c_set_speed(ser):
    cmd = "i2c set_speed 0 1"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if "Bus frequency set successfully" in output:
        print("✅ Success: i2c set speed test Passed")
    else:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c set speed test failed")


def i2c_get_speed(ser):
    cmd = "i2c get_speed 0"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if "Bus frequency is 100kHz" in output:
        print("✅ Success: i2c get speed test Passed")
    else:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c get speed test failed")


def i2c_write(ser):
    cmd = "i2c write i2c@7e7b0200 0x50 0x01 0x8"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)


def i2c_read(ser):
    cmd = "i2c scan i2c@7e7b0200"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    i2c_write(ser)
    cmd = "i2c read i2c@7e7b0200 0x50 0x1 0x02"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if "I2C tx success" in output:
        print("✅ Success: i2c read test Passed")
    else:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c read test failed")


def i2c_reg_write(ser):
    cmd = "i2c reg_write 0 0x48 0x01 0x2 0xAA 0xBB"
    output = serial_util.send_command_and_read(ser, cmd)
    if "BAD" in output:
        raise Exception(
            "❌ Failure: i2c Failed! Alert !!! the i2c reg write test failed")
    else:
        print("✅ Success: i2c reg write test Passed")


def run_all_tests(ser):
    import report_util
    import time
    try:
        i2c_scan0(ser)
        report_util.add_result("i2c scan0", "Success", "i2c scan 0 successful")
    except Exception as e:
        report_util.add_result("i2c scan0", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_scan1(ser)
        report_util.add_result("i2c scan1", "Success", "i2c scan 1 successful")
    except Exception as e:
        report_util.add_result("i2c scan1", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_scan3(ser)
        report_util.add_result("i2c scan3", "Success", "i2c scan3 successful")
    except Exception as e:
        report_util.add_result("i2c scan3", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_set_speed(ser)
        report_util.add_result("i2c set speed", "Success",
                               "i2c set speed successful")
    except Exception as e:
        report_util.add_result("i2c set speed", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_get_speed(ser)
        report_util.add_result("i2c get speed", "Success",
                               "i2c get speed successful")
    except Exception as e:
        report_util.add_result("i2c get speed", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_write(ser)
        report_util.add_result("i2c write", "Success", "i2c write successful")
    except Exception as e:
        report_util.add_result("i2c write", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_read(ser)
        report_util.add_result("i2c read", "Success", "i2c read successful")
    except Exception as e:
        report_util.add_result("i2c read", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_reg_read(ser)
        report_util.add_result("i2c reg read", "Success",
                               "i2c reg read successful")
    except Exception as e:
        report_util.add_result("i2c reg read", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_reg_write(ser)
        report_util.add_result("i2c reg write", "Success",
                               "i2c reg write successful")
    except Exception as e:
        report_util.add_result("i2c reg write", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_target_register(ser)
        report_util.add_result("i2c target register",
                               "Success", "i2c target register successful")
    except Exception as e:
        report_util.add_result("i2c target register", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i2c_target_get_address(ser)
        report_util.add_result("i2c target get addr",
                               "Success", "i2c target get addr successful")
    except Exception as e:
        report_util.add_result("i2c target get addr", "Failure", str(e))
        print(f"Exception: {e}")

# def i2c_reg_read(ser):
# #     cmd="i2c reg_read 0 0x48 0x01 0x2"
#     ser.write(cmd.encode())
#     ser.write(enter.encode())
#
#     output = ser.read(5*1024).decode('utf-8')
#     if "BAD" in output:
#         raise Exception("❌ Failure: i2c Failed! Alert !!! the i2c reg read test failed")
#     else:
#         print("✅ Success: i2c reg read test Passed")


# def i2c_target_register(ser):
# #     cmd="i2c target_register i2c@7e7b0200"
#     ser.write(cmd.encode())
#     ser.write(enter.encode())
#
#     output = ser.read(5*1024).decode('utf-8')
#     if "BAD" in output:
#         raise Exception("❌ Failure: i2c Failed! Alert !!! the i2c target reg test failed")
#     else:
#         print("✅ Success: i2c target reg test Passed")


# def i2c_target_get_address(ser):
# #     cmd="i2c target_get_address i2c@7e7b0200"
#     ser.write(cmd.encode())
#     ser.write(enter.encode())
#
#     output = ser.read(5*1024).decode('utf-8')
#     if "BAD" in output:
#         raise Exception("❌ Failure: i2c Failed! Alert !!! the i2c target get addr test failed")
#     else:
#         print("✅ Success: i2c target get addr test Passed")


# def i2c_target_set_address(ser):
# #     cmd="i2c target_set_address i2c@7e7b0200 0x50"
#     ser.write(cmd.encode())
#     ser.write(enter.encode())
#
#     output = ser.read(5*1024).decode('utf-8')
#     if "BAD" in output:
#         raise Exception("❌ Failure: i2c Failed! Alert !!! the i2c target set addr test failed")
#     else:
#         print("✅ Success: i2c target target set test Passed")


# def i2c_target_write(ser):
# #     cmd="i2c target_write i2c@7e7b0200 0x02 0xFF 0xFE"
#     ser.write(cmd.encode())
#     ser.write(enter.encode())
#
#     output = ser.read(5*1024).decode('utf-8')
#     print(output)


# def i2c_target_read(ser):
#     i2c_target_write(ser)
# #     cmd="i2c target_read i2c@7e7b0200 0x02"
#     ser.write(cmd.encode())
#     ser.write(enter.encode())
#
#     output = ser.read(5*1024).decode('utf-8')
#     print(output)
#
#     if "BAD" in output:
#         raise Exception("❌ Failure: i2c Failed! Alert !!! the i2c target read test failed")
#     else:
#         print("✅ Success: i2c target read test Passed")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    run_all_tests(ser)
    print("All I2C tests completed.")
    ser.close()
