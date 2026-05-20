import re
import configs  # Updated import
import serial_util
import sys
import report_util
import time

no_of_samples_updated = 0

def pmt_get_telemetry_precheck(ser):
    cmd = "pmt get_telemetry_precheck"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    expected_strings = [
        "Telemetry supported                 : 1",
        "Endpoint Capability                 : Telemetry API",
        "No of telemetry aggregators         : 3",
        "Aggregator details of OOBMSM_AGGR 1 -",
        "  Entry type                        : telemetry",
        "  Telemetry type                    : 0",
        # "  GUID                              : 0x5e2f8211",  # Do not check GUID value
        "  Telemetry space size              : 1024"
    ]
    # Check all expected strings except GUID
    if all(s in output for s in expected_strings):
        print("✅ Success: pmt get_telemetry_precheck test Passed")
    else:
        raise Exception(
            "❌ Failure: pmt get_telemetry_precheck Failed! Alert !!! the pmt get_telemetry_precheck test failed")

def pmt_get_static_telemetry(ser):
    cmd = "pmt get_static_telemetry"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    # Check for required sample ids and non-zero sample_buf values
    required_samples = {
        449, 450, 451, 453
    }
    found_samples = set()
    for line in output.splitlines():
        match = re.match(r"Sample id = (\d+)\s+sample_buf = (0x[0-9a-fA-F]+)", line)
        if match:
            sample_id = int(match.group(1))
            sample_buf = int(match.group(2), 16)
            if sample_id in required_samples:
                if sample_buf != 0:
                    found_samples.add(sample_id)
    if found_samples == required_samples:
        print("✅ Success: pmt get_static_telemetry test Passed")
    else:
        missing = required_samples - found_samples
        raise Exception(
            f"❌ Failure: pmt get_static_telemetry Failed! Missing or zero sample_buf for sample ids: {missing}")


def pmt_get_periodic_telemetry(ser):
    global no_of_samples_updated
    cmd = "pmt get_periodic_telemetry"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    # Check for "No of samples updated" and compare/update global variable

    required_samples = {
        1, 2, 7, 10, 16, 20, 21, 32, 34, 44, 45, 369, 372, 375, 378, 381, 384, 387, 390, 393, 396, 399, 402, 415, 416, 417, 454, 460
    }
    found_samples = set()
    latest_no_of_samples = 0
    for line in output.splitlines():
        # Check for "No of samples updated"
        match_count = re.match(r"No of samples updated:\s*(\d+)", line)
        if match_count:
            latest_no_of_samples = int(match_count.group(1))
            print(f"Latest no_of_samples_updated: {latest_no_of_samples}, Previous no_of_samples_updated: {no_of_samples_updated if 'no_of_samples_updated' in globals() else None}")
        # Check for sample id and sample_buf
        match = re.match(r"Sample id = (\d+)\s+sample_buf = (0x[0-9a-fA-F]+)", line)
        if match:
            sample_id = int(match.group(1))
            sample_buf = int(match.group(2), 16)
            if sample_id in required_samples:
                found_samples.add(sample_id)
    if latest_no_of_samples is None or latest_no_of_samples == 0:
        raise Exception("❌ Failure: No of samples updated not found in output!")
    if 'no_of_samples_updated' not in globals() or no_of_samples_updated is None:
        no_of_samples_updated = latest_no_of_samples
    elif latest_no_of_samples > no_of_samples_updated:
        no_of_samples_updated = latest_no_of_samples
    else:
        raise Exception(f"❌ Failure: Latest no_of_samples_updated ({latest_no_of_samples}) is not greater than previous value ({no_of_samples_updated})!")

    if found_samples >= required_samples:
        print("✅ Success: pmt get_periodic_telemetry test Passed")
    else:
        missing = required_samples - found_samples
        raise Exception(
            f"❌ Failure: pmt get_periodic_telemetry Failed! Missing sample ids: {missing}")


def pmt_run_telemetry(ser):
    cmd = "pmt run_telemetry"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "PMT periodic telemetry update resumed" in output:
        print("✅ Success: pmt run_telemetry test Passed")
    else:
        raise Exception(
            "❌ Failure: pmt run_telemetry Failed! Alert !!! the pmt run_telemetry test failed")


def pmt_pause_telemetry(ser):
    cmd = "pmt pause_telemetry"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "PMT periodic telemetry update paused" in output:
        print("✅ Success: pmt pause_telemetry test Passed")
    else:
        raise Exception(
            "❌ Failure: pmt pause_telemetry Failed! Alert !!! the pmt pause_telemetry test failed")

def run_all_tests(ser):
    try:
        pmt_get_telemetry_precheck(ser)
        report_util.add_result("pmt get_telemetry_precheck", "Success", "pmt get_telemetry_precheck successful")
    except Exception as e:
        report_util.add_result("pmt get_telemetry_precheck", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        pmt_get_static_telemetry(ser)
        report_util.add_result("pmt get_static_telemetry", "Success", "pmt get_static_telemetry successful")
    except Exception as e:
        report_util.add_result("pmt get_static_telemetry", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        pmt_pause_telemetry(ser)
        report_util.add_result("pmt pause_telemetry", "Success", "pmt pause_telemetry successful")
    except Exception as e:
        report_util.add_result("pmt pause_telemetry", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        pmt_run_telemetry(ser)
        report_util.add_result("pmt run_telemetry", "Success", "pmt run_telemetry successful")
    except Exception as e:
        report_util.add_result("pmt run_telemetry", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        pmt_get_periodic_telemetry(ser)
        report_util.add_result("pmt get_periodic_telemetry", "Success", "pmt get_periodic_telemetry successful")
    except Exception as e:
        report_util.add_result("pmt get_periodic_telemetry", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        pmt_pause_telemetry(ser)  # Pause telemetry before rechecking periodic telemetry
        time.sleep(1)
        pmt_run_telemetry(ser)  # run telemetry before rechecking periodic telemetry
        time.sleep(2)
        pmt_get_periodic_telemetry(ser)
        report_util.add_result("pmt pmt_get_periodic_telemetry after pause -> run", "Success", "pmt pmt_get_periodic_telemetry after pause->run successful")
    except Exception as e:
        report_util.add_result("pmt pmt_get_periodic_telemetry after pause -> run", "Failure", str(e))
        print(f"Exception: {e}")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    run_all_tests(ser)
    print("All PMT tests completed.")
    ser.close()