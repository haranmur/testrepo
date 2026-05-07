#Author: Haran Muru

import serial_util
import sys
import sensorparse
import time
import sched
import csv
import os
from datetime import datetime

def write_to_csv(sensor_data, filename="sensor_data.csv"):
    """Write sensor data to CSV file with one timestamp per set"""
    try:
        file_exists = os.path.exists(filename)
        
        with open(filename, 'a', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            
            # Only write header if file doesn't exist
            if not file_exists:
                writer.writerow(sensor_data[0])  # Write original header
            
            # Write one timestamp row for this set of readings
            current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            writer.writerow([f"--- Reading at {current_time} ---"])
            
            # Write all sensor data rows (without individual timestamps)
            for row in sensor_data[1:]:  # Skip header row
                writer.writerow(row)
        
        action = "appended to" if file_exists else "created"
        print(f"Sensor data {action} {filename}")
        return True
        
    except Exception as e:
        print(f"Error writing to CSV: {e}")
        return False
    
def clear_csv(filename):
    """Clear the contents of a CSV file"""
    try:
        with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
            pass  # Just opening in 'w' mode clears the file
        print(f"CSV file '{filename}' has been cleared")
        return True
    except Exception as e:
        print(f"Error clearing CSV file: {e}")
        return False

def sensor_write(ser):
    """Function to read sensors and write to CSV"""
    try:
        parsedata = sensorparse.list_numeric(ser)
        write_to_csv(parsedata)
    except Exception as e:
        print(f"Error in sensor_write: {e}")

def schedule_monitor(duration_seconds, ser):
    """Schedule sensor monitoring for specified duration"""

    clear_csv("sensor_data.csv")

    print(f"Starting sensor monitoring for {duration_seconds} seconds...")

    my_scheduler = sched.scheduler(time.time, time.sleep)
    
    # Schedule sensor reads for the specified duration
    for i in range(duration_seconds):
        # Pass the function reference and arguments correctly
        my_scheduler.enter(i + 1, 1, sensor_write, (ser,))
    
    try:
        # Run the scheduler
        my_scheduler.run()
    except KeyboardInterrupt:
        print("\nMonitoring interrupted by user")

if __name__ == "__main__":
    # Allow duration to be specified as command line argument
    duration = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    
    ser, result = serial_util.connect_serial()

    if ser is None:
        print(result["details"])
        sys.exit(1)

    schedule_monitor(duration, ser)
    ser.close()
    print("Sensor monitoring completed.")

# Monitor for default 10 seconds
#python sensormonitor.py

# Monitor for 30 seconds
#python sensormonitor.py 30