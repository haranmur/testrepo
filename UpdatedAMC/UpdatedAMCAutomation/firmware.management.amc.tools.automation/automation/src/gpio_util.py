import re
import configs  # Updated import
import serial_util
import sys


def parse_port_pins(text):
    lines = text.strip().split("\n")
    ports = {}
    # Extract header row (PIN names)
    # Extract pin headers (PIN0 - PIN7)
    headers = [h.strip() for h in lines[1].split("|")[2:-1]]
    # Process each row containing port data
    for line in lines[3:-1]:  # Skip first 3 header lines and last border line
        parts = line.split("|")
        if len(parts) < 10:
            continue
        port_name = parts[1].strip()  # Extract port name (e.g., PORT_A)
        pin_values = [p.strip() for p in parts[2:-1]]  # Extract pin values
        # Store port data in dictionary
        for i in range(len(headers)):
            ports[(port_name, headers[i])] = pin_values[i]
    return ports

def get_pin_value(parsed_ports, port, pin):
    return parsed_ports.get((port.strip(), pin.strip()), "Invalid port or pin")


def gpio_get(ser, port_name, pin_name):
    cmd = "gpio get"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    matrix_cleaned = output.replace('gpio get', '')
    print(matrix_cleaned)
    # Normalize text formatting
    matrix = matrix_cleaned.replace("\r\n", "\n").strip()
    # Parse matrix and display results
    parsed_ports = parse_port_pins(matrix)
    value = get_pin_value(parsed_ports, port_name, pin_name)
    print(f"Value at {port_name}, {pin_name}: {value}")


def gpio_set(ser, port_name, pin_name, value):
    cmd = "gpio set " + port_name + pin_name + value
    print("command = " + cmd)
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)


def run_all_tests(ser):
    import report_util
    try:
        gpio_get(ser, "PORT_E", "PIN2")
        report_util.add_result("gpio get", "Success", "gpio get successful")
    except Exception as e:
        report_util.add_result("gpio get", "Failure", str(e))
        print(f"Exception: {e}")
    try:
        gpio_set(ser, 'E ', '2 ', '0')
        report_util.add_result("gpio set", "Success",
                               "gpio set pin successful")
    except Exception as e:
        report_util.add_result("gpio set", "Failure", str(e))
        print(f"Exception: {e}")
    try:
        gpio_get(ser, "PORT_E", "PIN2")
        report_util.add_result("gpio get", "Success", "gpio get successful")
    except Exception as e:
        report_util.add_result("gpio get", "Failure", str(e))
        print(f"Exception: {e}")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    run_all_tests(ser)
    print("All GPIO tests completed.")
    ser.close()
