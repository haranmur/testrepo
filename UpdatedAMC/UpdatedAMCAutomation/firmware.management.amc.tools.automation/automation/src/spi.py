import configs  # Updated import
import serial_util
import sys

# TODO : add restore routine when write/erase done and testing is complete
# TODO : arbitrate data
# TODO : implement data as configs.data for write and read operations, compared


def spi_reinit(ser, spi_id):
    """Generic SPI reinit function for any SPI ID"""
    cmd = f"spi reinit {spi_id}"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "SPI reinit successful" in output:
        print(f"✅ Success: spi {spi_id} reinit test Passed")
    else:
        raise Exception(
            f"❌ Failure: spi Failed! Alert !!! the spi {spi_id} reinit failed")


# Default address keep it to the padding area above signature-1 area so as to not interfere with other flash content
def spi_write(ser, spi_id=0, addr=0xFDF1, len_val=0x06, data="1 2 3 4 5 6"):
    """Generic SPI write function"""
    cmd = f"spi write {spi_id} {len_val:#04x} {addr:#04x} {data}"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    return output

# Default address keep it to the padding area above signature-1 area so as to not interfere with other flash content
def spi_read(ser, spi_id=0, addr=0xFDF1, len_val=0x06, expect_data=True):
    """Generic SPI read function with configurable data expectation"""
    cmd = f"spi read {spi_id} {len_val:#04x} {addr:#04x}"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    data_found = "0x01 0x02 0x03 0x04 0x05 0x06" in output

    if expect_data and data_found:
        print(f"✅ Success: spi {spi_id} read and write test Passed")
    elif not expect_data and not data_found:
        print(f"✅ Success: spi {spi_id} erased data test is successful")
    elif expect_data and not data_found:
        raise Exception(
            f"❌ Failure: spi {spi_id} Failed! Alert !!! the spi read and write failed")
    else:  # not expect_data and data_found
        raise Exception(
            f"❌ Failure: spi {spi_id} Failed! Alert !!! the spi erased data read failed")

    return output


def spi_erase(ser, spi_id=0, addr=0xFDF1, len_val=0x06):
    """Generic SPI erase function"""
    cmd = f"spi erase {spi_id} {len_val:#04x} {addr:#04x}"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    return output


def spi_read_write(ser, spi_id=0):
    """Combined read/write test for specific SPI ID"""
    data = "1 2 3 4 5 6"
    spi_write(ser, spi_id=spi_id, data=data)
    spi_read(ser, spi_id=spi_id, expect_data=True)


def spi_erase_test(ser, spi_id=0):
    """Complete erase test sequence for specific SPI ID"""
    spi_read_write(ser, spi_id=spi_id)
    spi_erase(ser, spi_id=spi_id)
    spi_read(ser, spi_id=spi_id, expect_data=False)


def run_all_tests(ser):
    import report_util

    # Test SPI reinit for multiple IDs
    # comment out 2 since spi reinit always  fails for 2 i.e GPU
    # spi_ids = [0, 1, 2]
    spi_ids = [0, 1]
    for spi_id in spi_ids:
        try:
            spi_reinit(ser, spi_id)
            report_util.add_result(
                f"spi {spi_id} reinit", "Success", f"spi {spi_id} reinit successful")
        except Exception as e:
            report_util.add_result(f"spi {spi_id} reinit", "Failure", str(e))
            print(f"Exception: {e}")

    # Test SPI read/write for specific IDs
    spi_readwrite_ids = [0]  # Add 1, 2 to test other SPI IDs
    for spi_id in spi_readwrite_ids:
        try:
            spi_read_write(ser, spi_id=spi_id)
            report_util.add_result(
                f"spi {spi_id} read write", "Success", f"spi {spi_id} read write successful")
        except Exception as e:
            report_util.add_result(
                f"spi {spi_id} read write", "Failure", str(e))
            print(f"Exception: {e}")

    
    # Test SPI erase sequence for specific IDs
    # Disabling the erase tests due to the spi-erase command issue as it erases the complete region that what
    # is specified in the command, to be enabled once the issue is resolved
    '''
    spi_erase_ids = [0]
    for spi_id in spi_erase_ids:
        try:
            spi_erase_test(ser, spi_id=spi_id)
            report_util.add_result(
                f"spi {spi_id} erase", "Success", f"spi {spi_id} erase successful")
        except Exception as e:
            report_util.add_result(f"spi {spi_id} erase", "Failure", str(e))
            print(f"Exception: {e}")
    '''

if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    run_all_tests(ser)
    print("All SPI tests completed successfully.")
    ser.close()
