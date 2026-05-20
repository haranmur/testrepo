#Author: Haran Muru

import csv
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime
import sys
import re

def parse_csv_data(csv_file, sensor_number):
    """
    Parse CSV file and extract data for a specific sensor number
    
    Args:
        csv_file (str): Path to the CSV file
        sensor_number (int): The sensor number to plot
    
    Returns:
        tuple: (timestamps, values, sensor_info) or (None, None, None) if not found
    """
    timestamps = []
    values = []
    sensor_info = {}
    current_timestamp = None
    
    try:
        with open(csv_file, 'r', encoding='utf-8') as file:
            reader = csv.reader(file)
            
            for row in reader:
                # Skip empty rows
                if not row or len(row) == 0:
                    continue
                
                # Check if this is a timestamp row
                if row[0].startswith('--- Reading at'):
                    # Extract timestamp from "--- Reading at 2025-11-25 14:12:14 ---"
                    timestamp_match = re.search(r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})', row[0])
                    if timestamp_match:
                        current_timestamp = datetime.strptime(timestamp_match.group(1), '%Y-%m-%d %H:%M:%S')
                    continue
                
                # Check if this is sensor data (should have at least 6 columns)
                if len(row) >= 6 and current_timestamp:
                    try:
                        # Parse sensor number
                        sensor_nr = int(row[0])
                        
                        if sensor_nr == sensor_number:
                            # Found our sensor
                            sensor_name = row[1]
                            sensor_value = row[2]
                            sensor_lo = row[3]
                            sensor_hi = row[4]
                            sensor_unit = row[5]
                            sensor_status = row[6] if len(row) > 6 else ''
                            
                            # Store sensor info (will be the same for all readings)
                            if not sensor_info:
                                sensor_info = {
                                    'name': sensor_name,
                                    'unit': sensor_unit,
                                    'lo': sensor_lo,
                                    'hi': sensor_hi,
                                    'number': sensor_number
                                }
                            
                            # Try to convert value to float
                            try:
                                numeric_value = float(sensor_value)
                                timestamps.append(current_timestamp)
                                values.append(numeric_value)
                            except ValueError:
                                # Skip non-numeric values
                                continue
                                
                    except (ValueError, IndexError):
                        # Skip malformed rows
                        continue
        
        if not timestamps:
            return None, None, None
            
        return timestamps, values, sensor_info
        
    except FileNotFoundError:
        print(f"Error: CSV file '{csv_file}' not found.")
        return None, None, None
    except Exception as e:
        print(f"Error reading CSV file: {e}")
        return None, None, None

def plot_sensor_data(csv_file, sensor_number, output_file=None):
    """
    Plot sensor data for a specific sensor number
    
    Args:
        csv_file (str): Path to the CSV file
        sensor_number (int): Sensor number to plot
        output_file (str): Optional filename to save the plot
    """
    
    timestamps, values, sensor_info = parse_csv_data(csv_file, sensor_number)
    
    if timestamps is None:
        print(f"No data found for sensor number {sensor_number}")
        return False
    
    # Create the plot
    plt.figure(figsize=(12, 6))
    plt.plot(timestamps, values, marker='o', linestyle='-', markersize=4, linewidth=1.5)
    
    # Format the plot
    title = f'Sensor #{sensor_info["number"]}: {sensor_info["name"]}'
    plt.title(title, fontsize=14, fontweight='bold')
    plt.xlabel('Time', fontsize=12)
    plt.ylabel(f'Value ({sensor_info["unit"]})', fontsize=12)
    plt.grid(True, alpha=0.3)
    
    # Add threshold lines if available
    try:
        lo_threshold = float(sensor_info['lo'])
        hi_threshold = float(sensor_info['hi'])
        
        if lo_threshold > 0:
            plt.axhline(y=lo_threshold, color='orange', linestyle='--', alpha=0.7, label=f'Low Threshold: {lo_threshold}')
        if hi_threshold > 0:
            plt.axhline(y=hi_threshold, color='red', linestyle='--', alpha=0.7, label=f'High Threshold: {hi_threshold}')
        
        plt.legend()
    except ValueError:
        # Skip if thresholds are not numeric
        pass
    
    # Format x-axis to show time nicely
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
    if len(timestamps) > 10:
        plt.gca().xaxis.set_major_locator(mdates.SecondLocator(interval=max(1, len(timestamps)//10)))
    plt.xticks(rotation=45)
    
    # Add statistics
    if values:
        avg_value = sum(values) / len(values)
        min_value = min(values)
        max_value = max(values)
        
        stats_text = f'Statistics:\nAvg: {avg_value:.2f} {sensor_info["unit"]}\nMin: {min_value:.2f} {sensor_info["unit"]}\nMax: {max_value:.2f} {sensor_info["unit"]}\nSamples: {len(values)}'
        plt.text(0.02, 0.98, stats_text, transform=plt.gca().transAxes, 
                verticalalignment='top', bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))
    
    plt.tight_layout()
    
    # Save or show the plot
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Plot saved as {output_file}")
    else:
        plt.show()
    
    print(f"Plotted {len(values)} data points for sensor #{sensor_number} ({sensor_info['name']})")
    return True

def list_available_sensors(csv_file):
    """List all available sensor numbers and names from the CSV file"""
    sensors = {}
    
    try:
        with open(csv_file, 'r', encoding='utf-8') as file:
            reader = csv.reader(file)
            
            for row in reader:
                if len(row) >= 6 and not row[0].startswith('---'):
                    try:
                        sensor_nr = int(row[0])
                        sensor_name = row[1]
                        sensor_unit = row[5]
                        sensors[sensor_nr] = {'name': sensor_name, 'unit': sensor_unit}
                    except (ValueError, IndexError):
                        continue
        
        if sensors:
            print(f"Available sensors in {csv_file}:")
            print("-" * 60)
            for sensor_nr in sorted(sensors.keys()):
                print(f"{sensor_nr:2d}. {sensors[sensor_nr]['name']} ({sensors[sensor_nr]['unit']})")
        else:
            print("No sensors found in the CSV file.")
            
        return sensors
        
    except FileNotFoundError:
        print(f"Error: CSV file '{csv_file}' not found.")
        return {}
    except Exception as e:
        print(f"Error reading CSV file: {e}")
        return {}

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python sensorplot.py <csv_file> [sensor_number] [output_file]")
        print("       python sensorplot.py <csv_file> --list    (to list available sensors)")
        print("Example: python sensorplot.py sensor_data.csv 1")
        print("Example: python sensorplot.py sensor_data.csv 6 voltage_plot.png")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    
    # Check if user wants to list sensors
    if len(sys.argv) > 2 and sys.argv[2] == "--list":
        list_available_sensors(csv_file)
        sys.exit(0)
    
    if len(sys.argv) < 3:
        # If no sensor specified, list available sensors
        print("Available sensors:")
        list_available_sensors(csv_file)
        print("\nPlease specify a sensor number as the second argument.")
        sys.exit(1)
    
    try:
        sensor_number = int(sys.argv[2])
    except ValueError:
        print("Error: Sensor number must be an integer.")
        sys.exit(1)
    
    output_file = sys.argv[3] if len(sys.argv) > 3 else None
    
    plot_sensor_data(csv_file, sensor_number, output_file)

    #python sensorplot.py