import configs  # Updated import
import serial_util
import sys
import report_util


def fru_set_manufacturer(ser):
    # Use values from configs.py
    cmd = configs.fru_set_manufacturer_intel  # Update to use the new command
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.manufacturer_check in output:  # Update to use the new expected output
        print("✅ Success: fru set manufacture test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! fru set manufacture test failed")


def fru_set_product_series(ser):
    # Use values from configs.py
    cmd = configs.fru_set_product_series_melville_sound
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.product_series_check in output:
        print("✅ Success: fru set product series test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set product series test failed")


def fru_set_serial_number(ser):
    cmd = configs.fru_set_serial_number
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.serial_number_check in output:
        print("✅ Success: fru set serial number test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set serial number test failed")


def fru_set_part_number(ser):
    # Use values from configs.py
    cmd = configs.fru_set_part_number
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.part_number_check in output:
        print("✅ Success: fru set part number test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set part number test failed")


def fru_set_card_type(ser):
    # Use values from configs.py
    cmd = configs.fru_set_card_type
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.card_type_check in output:
        print("✅ Success: fru set card type test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set card type test failed")


def fru_set_tile_info(ser):
    # Use values from configs.py
    cmd = configs.fru_set_tile_info
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.tile_info_check in output:
        print("✅ Success: fru set tile info test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set tile info test failed")


def fru_set_platform_type(ser):
    # Use values from configs.py
    cmd = configs.fru_set_platform_type
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.platform_type_check in output:
        print("✅ Success: fru set platform type test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set platform type test failed")


def fru_set_fab_id(ser):
    # Use values from configs.py
    cmd = configs.fru_set_fab_id
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.fab_id_check in output:
        print("✅ Success: fru set fab id test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set fab id test failed")


def fru_set_product_name(ser):
    # Use values from configs.py
    cmd = configs.fru_set_product_name
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.product_name_check in output:
        print("✅ Success: fru set product name test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set product name test failed")


def fru_set_hw_rev(ser):
    cmd = configs.fru_set_hw_rev
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.hw_rev_check in output:
        print("✅ Success: fru set hw rev test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set hw rev test failed")


def fru_set_odm(ser):
    cmd = configs.fru_set_odm
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.odm_check in output:
        print("✅ Success: fru set odm test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set odm test failed")


def fru_set_card_tdp(ser):
    cmd = configs.fru_set_card_tdp
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.card_tdp_check in output:
        print("✅ Success: fru set card tdp test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set card tdp test failed")


def fru_set_uuid(ser):
    cmd = configs.fru_set_uuid
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.uuid_check in output:
        print("✅ Success: fru set uuid test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set uuid test failed")


def fru_set_crc(ser):
    cmd = configs.fru_set_crc
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.crc_check in output:
        print("✅ Success: fru set crc number test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set crc number test failed")


def fru_set_amc_slave_addr(ser):
    cmd = configs.fru_set_amc_slave_addr
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.amc_slave_addr_check in output:
        print("✅ Success: fru set amc slave addr test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set amc slave addr test failed")


def fru_set_fru_file_id(ser):
    cmd = configs.fru_set_fru_file_id
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.fru_file_id_check in output:
        print("✅ Success: fru set fru file id test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set fru file id test failed")


def fru_set_mfg_date(ser):
    cmd = configs.fru_set_mfg_date
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.mfg_date_check in output:
        print("✅ Success: fru set mfg date test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set mfg date failed")


def fru_set_rework_tracker(ser):
    cmd = configs.fru_set_rework_tracker
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.rework_tracker_check in output:
        print("✅ Success: fru set rework tracker test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set rework tracker test failed")


def fru_set_disable_write_fru(ser):
    cmd = configs.fru_set_disable_write_fru
    output = serial_util.send_command_and_read(ser, cmd)

    cmd = "fru display_general"
    output = serial_util.send_command_and_read(ser, cmd)
    print(output)

    if configs.disable_write_fru_check in output:
        print("✅ Success: fru set disable write test Passed")
    else:
        raise Exception(
            "❌ Failure: fru Failed! Alert !!! the fru set disable write test failed")


def run_all_tests(ser):
    try:
        fru_set_manufacturer(ser)
        report_util.add_result(
            "fru set manufacturer", "Success", "fru manufacturer successful")
    except Exception as e:
        report_util.add_result("fru set manufacturer", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_product_series(ser)
        report_util.add_result(
            "fru set product series", "Success", "fru product series successful")
    except Exception as e:
        report_util.add_result("fru set product series", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_serial_number(ser)
        report_util.add_result(
            "fru set serial number", "Success", "fru set serial number successful")
    except Exception as e:
        report_util.add_result("fru set serial number", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_part_number(ser)
        report_util.add_result(
            "fru set part number", "Success", "fru set part number successful")
    except Exception as e:
        report_util.add_result("fru set part number", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_card_type(ser)
        report_util.add_result(
            "fru set card type", "Success", "fru set card type successful")
    except Exception as e:
        report_util.add_result("fru set card type", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_tile_info(ser)
        report_util.add_result(
            "fru set tile info", "Success", "fru set tile info successful")
    except Exception as e:
        report_util.add_result("fru set tile info", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_platform_type(ser)
        report_util.add_result(
            "fru set platform type", "Success", "fru set platform type successful")
    except Exception as e:
        report_util.add_result("fru set platform type", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_fab_id(ser)
        report_util.add_result("fru set fab id", "Success",
                               "fru set fab id successful")
    except Exception as e:
        report_util.add_result("fru set fab id", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_product_name(ser)
        report_util.add_result(
            "fru set product name", "Success", "fru set product name successful")
    except Exception as e:
        report_util.add_result("fru set product name", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_hw_rev(ser)
        report_util.add_result(
            "fru set hw revision", "Success", "fru set hw revision successful")
    except Exception as e:
        report_util.add_result("fru set hw revision", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_odm(ser)
        report_util.add_result("fru set odm", "Success",
                               "fru set odm successful")
    except Exception as e:
        report_util.add_result("fru set odm", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_card_tdp(ser)
        report_util.add_result(
            "fru set card tdp", "Success", "fru set card tdp successful")
    except Exception as e:
        report_util.add_result("fru set card tdp", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_uuid(ser)
        report_util.add_result("fru set uuid", "Success",
                               "fru set uuid successful")
    except Exception as e:
        report_util.add_result("fru set uuid", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_crc(ser)
        report_util.add_result("fru set crc", "Success",
                               "fru set crc successful")
    except Exception as e:
        report_util.add_result("fru set crc", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_amc_slave_addr(ser)
        report_util.add_result(
            "fru set amc slave addr", "Success", "fru set amc slave addr successful")
    except Exception as e:
        report_util.add_result("fru set amc slave addr", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_fru_file_id(ser)
        report_util.add_result("fru set fru file id",
                               "Success", "fru set file id successful")
    except Exception as e:
        report_util.add_result("fru set fru file id", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_mfg_date(ser)
        report_util.add_result(
            "fru set mfg date", "Success", "fru set mfg date successful")
    except Exception as e:
        report_util.add_result("fru set mfg date", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_rework_tracker(ser)
        report_util.add_result(
            "fru set rework tracker", "Success", "fru rework tracker successful")
    except Exception as e:
        report_util.add_result("fru set rework tracker", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        fru_set_disable_write_fru(ser)
        report_util.add_result(
            "fru set disable write", "Success", "fru disable write successful")
    except Exception as e:
        report_util.add_result("fru set disable write", "Failure", str(e))
        print(f"Exception: {e}")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)

    run_all_tests(ser)
    print("All FRU tests completed.")
    ser.close()
