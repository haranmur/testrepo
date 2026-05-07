#Author: Haran Muru

import configs  # Updated import
import serial_util
import sys
import re

def parse_sensor_data(raw_output):
    """Parse the sensor output into a structured array"""
    sensors = []
    lines = raw_output.strip().split('\n')
    
    # Find the start of data (skip header lines)
    data_start = 0
    for i, line in enumerate(lines):
        if 'NR' in line and 'NAME' in line and 'VALUE' in line:
            data_start = i + 2  # Skip header and separator line
            break
    
    # Add header row first
    headers = ['nr', 'name', 'value', 'lo', 'hi', 'unit', 'status']
    sensors.append(headers)
    
    # Parse each sensor line
    for line in lines[data_start:]:
        line = line.strip()
        if not line or line.startswith('-') or line.startswith('['):
            continue
            
        # Split the line into components (handling multiple spaces)
        parts = re.split(r'\s{2,}', line.strip())
        
        if len(parts) >= 6:  # Ensure we have all required fields
            try:
                sensor = [
                    int(parts[0]) if parts[0].isdigit() else parts[0],  # nr
                    parts[1],  # name
                    parts[2],  # value
                    parts[3],  # lo
                    parts[4],  # hi
                    parts[5],  # unit
                    parts[6] if len(parts) > 6 else ''  # status
                ]
                sensors.append(sensor)
            except (ValueError, IndexError):
                continue  # Skip malformed lines
    
    return sensors

# Modify your existing list_numeric function:
def list_numeric(ser):
    cmd = "sensor list_numeric"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    return parse_sensor_data(output)  # Return parsed data instead of printing

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
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)

    print(run_all_tests(ser))
    ser.close()
    print("Sensor data has been parsed.")

#python sensorparse.py