import configs  # Updated import
import serial_util
import sys
import pmt
import peci

def i3c_scan(ser):
    cmd = "i3c scan 1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "1 devices found on bus 1" in output:
        print("✅ Success: i3c scan test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c scan test failed")


def i3c_write(ser):
    cmd = "i3c write 1 0x8 0x10 0x01 0xE1 0xE0 0xc8 0x7e 0x80 0x86 0x80 0x05 0x01 0x01 0x00 0x00 0x00 0x00 0x00 0xF8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "I3C tx success" in output:
        print("✅ Success: i3c write test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c write test failed")


def i3c_broadcast_enec(ser):
    cmd = "i3c broadcast_ccc_enec 1 0xb"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "ENEC (0x01) successful" in output:
        print("✅ Success: i3c broadcast enec test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! i3c broadcast enec test failed")


def i3c_broadcast_disec(ser):
    cmd = "i3c broadcast_ccc_disec 1 0xb"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "DISEC (0x02) successful" in output:
        print("✅ Success: i3c broadcast disec test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! i3c broadcast disec test failed")


def i3c_direct_ccc_enec(ser):
    cmd = "i3c direct_ccc_enec 1 8 0xb"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "CCC: ENEC (0x80) successful" in output:
        print("✅ Success: i3c direct ccc enec test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! i3c direct ccc enec test failed")


def i3c_direct_ccc_disec(ser):
    cmd = "i3c direct_ccc_disec 1 8 0xb"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "CCC: DISEC (0x81) successful" in output:
        print("✅ Success: i3c direct ccc disec test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! i3c direct ccc disec test failed")


def i3c_direct_ccc_getdeviceinfo(ser):
    cmd = "i3c direct_ccc_getdeviceinfo 1 0x8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "Device get basic info successful" in output:
        print("✅ Success:i3c get device info  test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c get device info test failed")


def i3c_direct_ccc_getstatus(ser):
    cmd = "i3c direct_ccc_getstatus 1 0x8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "CCC: GETSTATUS (0x90) successful" in output:
        print("✅ Success:i3c get status test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c get status test failed")


def i3c_direct_ccc_getdcr(ser):
    cmd = "i3c direct_ccc_getdcr 1 0x8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: GETDCR (0x8F) successful" in output:
        print("✅ Success:i3c get dcr test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c get dcr test failed")


def i3c_direct_ccc_getbcr(ser):
    cmd = "i3c direct_ccc_getbcr 1 8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if ("CCC: GETBCR (0x8E) successful" in output):
        print("✅ Success:i3c get bcr test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c get bcr test failed")


def i3c_direct_ccc_setmrl(ser):
    cmd = "i3c direct_ccc_setmrl 1 8 1 1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: SETMRL (0x8A) successful" in output:
        print("✅ Success:i3c set direct ccc mrlget test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c direct ccc set mrl test failed")


def i3c_direct_ccc_setmwl(ser):
    cmd = "i3c direct_ccc_setmwl 1 8 1 1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: SETMWL (0x89) successful" in output:
        print("✅ Success:i3c set direct ccc mwl test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c set direct ccc mwl test failed")


def i3c_broadcast_ccc_setmrl(ser):
    cmd = "i3c broadcast_ccc_setmrl 1 8 16 4"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: SETMRL (0x0A) successful" in output:
        print("✅ Success:i3c broadcast ccc set mrl test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c broadcast ccc set mrl test failed")


def i3c_broadcast_ccc_rstdaa(ser):
    cmd = "i3c broadcast_ccc_rstdaa 1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: RSTDAA (0x06) successful" in output:
        print("✅ Success:i3c broadcast ccc rstdaa test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c broadcast ccc rstdaa test failed")


def i3c_broadcast_ccc_entdaa(ser):
    cmd = "i3c broadcast_ccc_entdaa 1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "ENTDAA (0x07) successful" in output:
        print("✅ Success:i3c broadcast ccc entdaa test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c broadcast ccc entdaa test failed")


def i3c_direct_ccc_setnewda(ser):
    cmd = "i3c direct_ccc_setnewda 1 8 8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: SETNEWDA (0x88) successful" in output:
        print("✅ Success:i3c direct ccc setnewda test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c direct ccc setnewda test failed")


def i3c_direct_ccc_getmrl(ser):
    cmd = "i3c direct_ccc_getmrl 1 0x8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: SETAASA (0x29) successful" in output:
        print("✅ Success:i3c direct ccc getmrl test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c direct ccc getmrl test failed")


def i3c_direct_ccc_getmwl(ser):
    cmd = "i3c direct_ccc_getmwl 1 0x8"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "CCC: SETAASA (0x29) successful" in output:
        print("✅ Success:i3c direct ccc getmwl test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c direct ccc getmwl test failed")


# def i3c_direct_ccc_setdasa(ser):
# #     cmd="i3c direct_ccc_setdasa 1 8"
#     ser.write(cmd.encode())
#     ser.write(enter.encode())
#     time.sleep(1)
#     output = ser.read(5*1024).decode('utf-8')
#     print(output)
#     time.sleep(1)
#     if "CCC: SETAASA (0x29) successful" in output:
# 	    print("✅ Success:i3c direct ccc setdasa test Passed")
#     else:
#         raise Exception("❌ Failure: i3c Failed! Alert !!! the i3c direct ccc setdasa test failed")


def i3c_read(ser):
    cmd = "i3c read 1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)
    if "I3C rx success" in output:
        print("✅ Success:i3c read test Passed")
    else:
        raise Exception(
            "❌ Failure: i3c Failed! Alert !!! the i3c read test failed")

#### TODO: enable when peci/pmt used
## def i3c_pmt_pause(ser):
##     cmd = "pmt pause_telemetry"
##     output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
##     print(output)
##     if "PMT periodic telemetry update paused" in output:
##         print("✅ Success:i3c pmt pause test Passed")
##
##
## def i3c_peci_pause(ser):
##     cmd = "peci pause_gpu_update"
##     output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
##     print(output)
##     if "PECI periodic gpu update paused" in output:
##         print("✅ Success:i3c peci pause test Passed")


def run_all_tests(ser):
    import report_util

    ## TODO: enable when peci/pmt used
    ## i3c_pmt_pause(ser)
    ## i3c_peci_pause(ser)

    try:
        i3c_scan(ser)
        report_util.add_result("i3c scan", "Success", "i3c scan successful")
    except Exception as e:
        report_util.add_result("i3c scan check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        pmt.pmt_pause_telemetry(ser)
        peci.peci_pause_gpu_update(ser)
        i3c_write(ser)
        report_util.add_result("i3c Write", "Success", "i3c write successful")
    except Exception as e:
        report_util.add_result("i3c write check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_read(ser)
        report_util.add_result("i3c read", "Success", "i3c read successful")
    except Exception as e:
        report_util.add_result("i3c read", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_broadcast_enec(ser)
        report_util.add_result("i3c broadcast_enec", "Success",
                               "i3c broadcast_enec successful")
    except Exception as e:
        report_util.add_result("i3c broadcast_enec check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_broadcast_disec(ser)
        report_util.add_result("i3c broadcast disec", "Success",
                               "i3c_broadcast_disec successful")
    except Exception as e:
        report_util.add_result(
            "i3c i3c_broadcast_disec check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_enec(ser)
        report_util.add_result("i3c direct ccc_enec", "Success",
                               "i3c_direct_ccc_enec successful")
    except Exception as e:
        report_util.add_result(
            "i3c i3c_direct_ccc_enec check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_disec(ser)
        report_util.add_result("i3c i3c_direct_ccc_disec", "Success",
                               "i3c_direct_ccc_disec successful")
    except Exception as e:
        report_util.add_result(
            "i3c i3c_direct_ccc_disec check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_getstatus(ser)
        report_util.add_result("i3c i3c_direct_ccc_getstatus", "Success",
                               "i3c_direct_ccc_getstatus successful")
    except Exception as e:
        report_util.add_result(
            "i3c  i3c_direct_ccc_getstatus check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_getdcr(ser)
        report_util.add_result("i3c  i3c_direct_ccc_getdcr", "Success",
                               "i3c_direct_ccc_getdcr successful")
    except Exception as e:
        report_util.add_result(
            "i3c  i3c_direct_ccc_getdcr check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_getbcr(ser)
        report_util.add_result("i3c i3c_direct_ccc_getbcr", "Success",
                               "i3c_direct_ccc_getbcr successful")
    except Exception as e:
        report_util.add_result(
            "i3c  i3c_direct_ccc_getbcr check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_setmrl(ser)
        report_util.add_result("i3c  i3c_direct_ccc_setmrl", "Success",
                               "i3c_direct_ccc_setmrl successful")
    except Exception as e:
        report_util.add_result(
            "i3c  i3c_direct_ccc_setmrl check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_setmwl(ser)
        report_util.add_result("i3c  i3c_direct_ccc_setmwl", "Success",
                               "i3c_direct_ccc_setmwl successful")
    except Exception as e:
        report_util.add_result(
            "i3c i3c_direct_ccc_setmwl check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_broadcast_ccc_setmrl(ser)
        report_util.add_result("i3c i3c_broadcast_ccc_setmrl", "Success",
                               "i3c_broadcast_ccc_setmrl successful")
    except Exception as e:
        report_util.add_result(
            "i3c  i3c_broadcast_ccc_setmrl check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_broadcast_ccc_rstdaa(ser)
        report_util.add_result("i3c i3c_broadcast_ccc_rstdaa", "Success",
                               "i3c_broadcast_ccc_rstdaa successful")
    except Exception as e:
        report_util.add_result(
            "i3c i3c_broadcast_ccc_rstdaa check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_broadcast_ccc_entdaa(ser)
        report_util.add_result("i3c i3c_broadcast_ccc_entdaa", "Success",
                               "i3c_broadcast_ccc_entdaa successful")
    except Exception as e:
        report_util.add_result(
            "i3c i3c_broadcast_ccc_entdaa check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_setnewda(ser)
        report_util.add_result("i3c i3c_direct_ccc_setnewda", "Success",
                               "i3c_direct_ccc_setnewda successful")
    except Exception as e:
        report_util.add_result(
            "i3c i3c_direct_ccc_setnewda check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_getmrl(ser)
        report_util.add_result("i3c direct_ccc_getmrl", "Success",
                               "i3c direct_ccc_getmrl successful")
    except Exception as e:
        report_util.add_result(
            "i3c direct_ccc_getmrl check", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        i3c_direct_ccc_getmwl(ser)
        report_util.add_result("i3c direct_ccc_getmwl", "Success",
                               "i3c direct_ccc_getmwl successful")
    except Exception as e:
        report_util.add_result(
            "i3c direct_ccc_getmwl check", "Failure", str(e))
        print(f"Exception: {e}")

    # TODO: Uncomment when implemented
    ##try:
    ##    i3c_direct_ccc_setdasa(ser)
    ##    report_util.add_result("i3c direct_ccc_setdasa",
    ##                           "Success", "i3c direct_ccc_setdasa successful")
    ##except Exception as e:
    ##    report_util.add_result(
    ##        "i3c direct_ccc_setdasa check", "Failure", str(e))


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    run_all_tests(ser)
    print("All I3C tests completed.")
    ser.close()
