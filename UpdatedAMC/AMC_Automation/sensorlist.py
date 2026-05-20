#Author: Haran Muru

import configs  # Updated import
import serial_util
import sys
import re

def parse_sensor_data(raw_output):
    """Parse the sensor output and extract only sensor names"""
    sensor_names = []
    lines = raw_output.strip().split('\n')
    
    # Find the start of data (skip header lines)
    data_start = 0
    for i, line in enumerate(lines):
        if 'NR' in line and 'NAME' in line and 'VALUE' in line:
            data_start = i + 2  # Skip header and separator line
            break
    
    # Parse each sensor line and extract only the name
    for line in lines[data_start:]:
        line = line.strip()
        if not line or line.startswith('-') or line.startswith('['):
            continue
            
        # Split the line into components (handling multiple spaces)
        parts = re.split(r'\s{2,}', line.strip())
        
        if len(parts) >= 6:  # Ensure we have all required fields
            try:
                sensor_names.append(parts[1])  # Only append the name (parts[1])
            except (ValueError, IndexError):
                continue  # Skip malformed lines
    
    return sensor_names

def list_numeric(ser):
    cmd = "sensor list_numeric"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    return parse_sensor_data(output)  # Return parsed sensor names

def bmcSensorPull(ser):
    cmd = "busctl tree xyz.openbmc_project.pldm"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    return output

if __name__ == "__main__":
    print("Enter AMC Serial Port id (i.e. COM4)")
    amcPort = input()
    amcPort = amcPort.strip().upper()

    print("\nEnter BMC Serial Port id (i.e. COM4)")
    bmcPort = input()
    bmcPort = bmcPort.strip().upper()

    if not amcPort.startswith('COM') or not bmcPort.startswith('COM'):
        print("False serial port entry")
        sys.exit(1)
    else:
        print("\nStarting sensor test for port: " + amcPort + "\n")
        ser, result = serial_util.connect_serial(amcPort)

        if ser is None:
            print(result["details"])
            sys.exit(1)
        try:
            amc_sensor_names = list_numeric(ser)
            print(amc_sensor_names)

        except Exception as e:
            print(str(e))
            sys.exit(1)
        ser.close()

        serbmc, resbmc = serial_util.connect_serial(bmcPort)
        if serbmc is None:
            print(resbmc["details"])
            sys.exit(1)
        try:
            print(bmcSensorPull(serbmc))
            #ENTER BMC PORTION OF TEST HERE
        
        except Exception as e:
            print(str(e))
            sys.exit(1)

        serbmc.close()