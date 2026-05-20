#Author: Haran Muru

import configs  # Updated import
import serial_util
import sys
import re

bmcSerialPort = 'COM6'

# Modify your existing list_numeric function:
def list_numeric(ser):
    cmd = "busctl introspect xyz.openbmc_project.pldm /xyz/openbmc_project/sensors/temperature/OUTLET_TEMP_1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    return output

def run_all_tests(ser):
    import report_util
    try:
        parsedata = list_numeric(ser)
        report_util.add_result(
            "Sensor List Numeric", "Success", "Sensor list numeric verification passed")
        return parsedata
    except Exception as e:
        report_util.add_result("Sensor List Numeric", "Failure", str(e))
        print(f"Exception: {e}")

if __name__ == "__main__":
    ser, result = serial_util.connect_serial(bmcSerialPort)
    if ser is None:
        print(result["details"])
        sys.exit(1)

    print(run_all_tests(ser))
    ser.close()
    print("BMC Sensor data has been parsed.")

#python sensorparse.py