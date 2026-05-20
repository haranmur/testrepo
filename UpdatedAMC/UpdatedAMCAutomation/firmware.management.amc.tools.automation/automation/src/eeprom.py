import configs  # Updated import
import serial_util
import sys


def eeprom_initial_read(ser):
    """Read 16 bytes of data from EEPROM at offset 0x10 before writing."""
    cmd = "eeprom read 0x10 16"  # Read 16 bytes from offset 0x10
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print("Initial EEPROM data:", output)
    return output

# TODO : add restore routine when write/erase done and testing is complete
# TODO : arbitrate data
# TODO : implement data as configs.data for write and read operations, compared


def eeprom_write(ser):
    """Write data to EEPROM at offset 0x10."""
    cmd = "eeprom write 0x10 4 a b c d "  # Write data to offset 0x10
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)


def eeprom_write_and_verify(ser):
    """Write data to EEPROM and read back to verify."""
    initial_data = eeprom_initial_read(
        ser)  # Perform the initial read operation
    print("Initial data before writing:", initial_data)

    eeprom_write(ser)  # Perform the write operation

    cmd = "eeprom read 0x10 16"  # Read 16 bytes from offset 0x10 again
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(f"Data after writing:{output} {type(output)}")

    if "0x0a 0x0b 0x0c 0x0d" in output:
        print("✅ Success: EEPROM write and read test Passed")
    else:
        raise Exception(
            "❌ Failure: EEPROM Failed! Alert !!! the EEPROM write and read test failed")


def run_all_tests(ser):
    import report_util
    try:
        eeprom_write_and_verify(ser)
        report_util.add_result("EEPROM write and verify",
                              "Success", "EEPROM write and read successful")
    except Exception as e:
        report_util.add_result("EEPROM write and verify", "Failure", str(e))
        print(f"Exception: {e}")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    run_all_tests(ser)
    print("All EEPROM tests completed.")
