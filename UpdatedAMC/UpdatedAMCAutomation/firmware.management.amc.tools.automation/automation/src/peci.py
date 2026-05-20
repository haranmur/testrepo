import re
import configs  # Updated import
import serial_util
import sys
import report_util
import time

no_of_samples_updated = 0

def peci_get_endpt_localpci_cfg(ser):
    cmd = "peci get_endpt_localpci_cfg"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    configs_found = {
        "SSDID": None,
        "SSVID": None,
        "DID": None,
        "VID": None,
        "PCIE_LINK_SPEED": None,
        "PCIE_LINK_WIDTH": None,
    }

    for line in output.splitlines():
        # Check for endpoint localpci config lines
        match_cfg = re.match(r"endpoint localpci config of (\w+)=\s*(0x[0-9a-fA-F]+)", line)
        if match_cfg:
            key = match_cfg.group(1)
            value = match_cfg.group(2)
            if key in configs_found:
                configs_found[key] = value

    # Expected values
    expected_configs = {
        "SSDID": "0x1500",
        "SSVID": "0x8086",
        "DID": "0xe216",
        "VID": "0x8086",
        "PCIE_LINK_SPEED": "0x400c11",
        "PCIE_LINK_WIDTH": "0x400c11",
    }

    missing = [k for k, v in configs_found.items() if v != expected_configs[k]]
    if not missing:
        print("✅ Success: peci get_endpt_localpci_cfg test Passed")
    else:
        raise Exception(
            f"❌ Failure: peci get_endpt_localpci_cfg Failed! Missing or incorrect configs: {missing}, found: {configs_found}"
        )

def peci_get_pkg_cfg(ser):
    global no_of_samples_updated
    cmd = "peci get_pkg_cfg"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    # Parse "No of samples updated" and pkg config lines

    latest_no_of_samples = None
    configs_found = {
        "PKG_MAX_TEMP": None,
        "PACKAGE_RAPL_PERF_STATUS": None,
        "THERM_MARGIN": None,
        "TEMP_TARGET": None,
        "PKG_THERM_STATUS": None,
        "PKG_RAPL_LIMIT1": None,
        "PKG_RAPL_LIMIT2": None,
        "PKG_POWER_SKU_LOW": None,
        "PKG_POWER_SKU_HIGH": None,
        "PKG_POWER_SKU_UNIT": None,
        "PSYSGPU_RAPL_LIMIT1": None,
        "PSYSGPU_RAPL_LIMIT2": None,
        "PKG_PL4_LIMIT": None,
        "PERF_LIMIT_REASONS": None,
        "SET_PSYSGPU_CRIT_THRESHOLD": None,
        "PLATFORM_PL4_OFFSET": None,
    }

    for line in output.splitlines():
        # Check for "No of samples updated"
        match_count = re.match(r"No of samples updated:\s*(\d+)", line)
        if match_count:
            latest_no_of_samples = int(match_count.group(1))
            print(f"Latest no_of_samples_updated: {latest_no_of_samples}, Previous no_of_samples_updated: {no_of_samples_updated if 'no_of_samples_updated' in globals() else None}")

        # Check for pkg config lines
        match_cfg = re.match(r"pkg config of (\w+)=\s*(0x[0-9a-fA-F]+)", line)
        if match_cfg:
            key = match_cfg.group(1)
            value = match_cfg.group(2)
            if key in configs_found:
                configs_found[key] = value

    if latest_no_of_samples is None or latest_no_of_samples == 0:
        raise Exception("❌ Failure: No of samples updated not found in output!")

    if 'no_of_samples_updated' not in globals() or no_of_samples_updated is None:
        no_of_samples_updated = latest_no_of_samples
    elif latest_no_of_samples > no_of_samples_updated:
        no_of_samples_updated = latest_no_of_samples
    else:
        raise Exception(f"❌ Failure: Latest no_of_samples_updated ({latest_no_of_samples}) is not greater than previous value ({no_of_samples_updated})!")

    # Only check for presence of all required configs (not their values)
    missing = [k for k, v in configs_found.items() if v is None]
    if not missing:
        print("✅ Success: peci get_get_pkg_cfg test Passed (all configs present)")
    else:
        raise Exception(
            f"❌ Failure: peci get_get_pkg_cfg Failed! Missing configs: {missing}, found: {configs_found}"
        )

def peci_pause_gpu_update(ser):
    cmd = "peci pause_gpu_update"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "PECI periodic gpu update paused" in output:
        print("✅ Success: peci pause_gpu_update test Passed")
    else:
        raise Exception(
            "❌ Failure: peci pause_gpu_update Failed! Alert !!! the peci pause_gpu_update test failed")


def peci_run_gpu_update(ser):
    cmd = "peci run_gpu_update"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    if "PECI periodic gpu update resumed" in output:
        print("✅ Success: peci run_gpu_update test Passed")
    else:
        raise Exception(
            "❌ Failure: peci run_gpu_update Failed! Alert !!! the peci run_gpu_update test failed")


def peci_get_gpu_svn(ser):
    cmd = "peci get_gpu_svn"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    match = re.search(r"GPU SVN\s*=\s*0x[0-9a-fA-F]+", output)
    if match:
        print("✅ Success: peci get_gpu_svn test Passed")
    else:
        raise Exception(
            "❌ Failure: peci get_gpu_svn Failed! Alert !!! the peci get_gpu_svn test failed")

def run_all_tests(ser):
    try:
        peci_pause_gpu_update(ser)
        report_util.add_result("peci pause_gpu_update", "Success", "peci pause_gpu_update successful")
    except Exception as e:
        report_util.add_result("peci pause_gpu_update", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        peci_run_gpu_update(ser)
        report_util.add_result("peci run_gpu_update", "Success", "peci run_gpu_update successful")
    except Exception as e:
        report_util.add_result("peci run_gpu_update", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        peci_get_endpt_localpci_cfg(ser)
        report_util.add_result("peci get_endpt_localpci_cfg", "Success", "peci get_endpt_localpci_cfg successful")
    except Exception as e:
        report_util.add_result("peci get_endpt_localpci_cfg", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        peci_get_pkg_cfg(ser)
        report_util.add_result("peci get_get_pkg_cfg", "Success", "peci get_get_pkg_cfg successful")
    except Exception as e:
        report_util.add_result("peci get_get_pkg_cfg", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        time.sleep(1)
        peci_get_pkg_cfg(ser)  # Recheck after pause and run
        peci_pause_gpu_update(ser)  # Pause telemetry before rechecking periodic telemetry
        time.sleep(1)
        peci_run_gpu_update(ser)  # run telemetry before rechecking periodic telemetry
        peci_get_pkg_cfg(ser)
        report_util.add_result("peci get_get_pkg_cfg after pause -> run", "Success", "peci get_get_pkg_cfg after pause->run successful")
    except Exception as e:
        report_util.add_result("peci get_get_pkg_cfg after pause -> run", "Failure", str(e))
        print(f"Exception: {e}")

    try:
        peci_get_gpu_svn(ser)
        report_util.add_result("peci get_gpu_svn", "Success", "peci get_gpu_svn successful")
    except Exception as e:
        report_util.add_result("peci get_gpu_svn", "Failure", str(e))
        print(f"Exception: {e}")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    run_all_tests(ser)
    print("All PECI tests completed.")
    ser.close()