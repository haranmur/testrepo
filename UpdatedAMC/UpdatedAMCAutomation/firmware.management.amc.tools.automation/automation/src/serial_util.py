import serial # type: ignore
import configs  # Updated import
import report_util
import datetime
import threading
import os
import time
import serial.tools.list_ports

SERIAL_LOG_DT_STR = None
SERIAL_LOG_FILE = None
SERIAL_LOG_LOCK = threading.Lock()
SERIAL_LOG_ENABLED = True

def listPorts():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

def set_serial_log_enabled(enabled: bool):
    global SERIAL_LOG_ENABLED
    SERIAL_LOG_ENABLED = enabled


def get_serial_log_dt_str():
    """
    Returns the session datetime string for log/report filenames.
    Generates it once per session and reuses it.
    """
    global SERIAL_LOG_DT_STR
    if SERIAL_LOG_DT_STR is None:
        # Format: YYYYMMDD_HHMMSS
        SERIAL_LOG_DT_STR = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    return SERIAL_LOG_DT_STR


def get_serial_log_file():
    global SERIAL_LOG_FILE
    if SERIAL_LOG_FILE is None:
        dt_str = get_serial_log_dt_str()
        results_dir = "/results"
        if not os.path.exists(results_dir):
            results_dir = os.path.join(os.getcwd(), "results")
        os.makedirs(results_dir, exist_ok=True)
        SERIAL_LOG_FILE = os.path.join(
            results_dir, f"console_log_{dt_str}.txt")
    return SERIAL_LOG_FILE


def log_serial(direction, data):
    if not SERIAL_LOG_ENABLED:
        return
    with SERIAL_LOG_LOCK:
        logfile_path = get_serial_log_file()
        timestamp = datetime.datetime.now().strftime(
            "%Y-%m-%d %H:%M:%S.%f")[:-3]  # Include milliseconds
        color_code = "\033[33m" if direction == "READ" else "\033[36m" if direction == "WRITE" else ""
        reset_code = "\033[0m"
        with open(logfile_path, "a", encoding="utf-8") as logfile:
            logfile.write(
                f"[{timestamp}] [{color_code}{direction}{reset_code}] {data}\n")


class LoggedSerial:
    def __init__(self, ser):
        self.ser = ser

    def write(self, data):
        log_serial("WRITE", data.decode('utf-8', errors='replace'))
        # Write all data at once, retry if not all bytes written
        total_written = 0
        max_attempts = 2
        attempts = 0
        while total_written < len(data) and attempts < max_attempts:
            try:
                written = self.ser.write(data[total_written:])
                self.ser.flush()
                total_written += written
                if written == 0:
                    attempts += 1
                    time.sleep(0.05)
            except Exception as e:
                print(f"Serial write error: {e}")
                return total_written
        if total_written < len(data):
            print(
                f"Warning: Only wrote {total_written} of {len(data)} bytes after {max_attempts} attempts")
        return total_written

    def read(self, size=1, big_timeout=6, small_timeout=0.15):
        """
            Adaptive read: waits for data up to 2 seconds, then waits 0.5s after last data chunk.
        """
        data = b""
        start_time = time.time()
        last_data_time = start_time
        while True:
            if self.ser.in_waiting > 0:
                chunk = self.ser.read(self.ser.in_waiting)
                data += chunk
                last_data_time = time.time()
            else:
                if data and (time.time() - last_data_time > small_timeout):
                    break
                if not data and (time.time() - start_time > 5):
                    break
                time.sleep(0.1)
        try:
            log_serial("READ", data.decode('utf-8', errors='replace'))
        except Exception:
            log_serial("READ", str(data))
        return data

    def __getattr__(self, attr):
        return getattr(self.ser, attr)


def send_command_and_read(ser, cmd="", enter=None, size=None, timeout=0.3):
    """
    Send a command to the serial port and read the response.

    Args:
        ser: Serial connection object
        cmd: Command string to send
        enter: Enter character/string (defaults to configs.enter_1)
        size: Number of bytes to read (defaults to configs.size)
        timeout: Timeout for reading full buffer (defaults to 0.1 seconds)

    Returns:
        str: Decoded response from the serial port
    """
    if enter is None:
        enter = configs.enter_1
    if size is None:
        size = configs.size
    ser.write((cmd + enter).encode())
    output = ser.read(size, small_timeout=timeout).decode('utf-8', errors='ignore')

    return output


def check_amc_prompt(ser):

    try:
        # Use send_command_and_read with empty command to trigger prompt
        response = send_command_and_read(ser)

        # Simple string comparison for AMC prompt
        if "amc:~$" in response:
            print("✅ AMC prompt 'amc:~$' found!")
            return True
        else:
            print("❌ AMC prompt 'amc:~$' not found.")
            print(f"response:{response}")
            return False

    except Exception as e:
        print(f"⚠️  Error during prompt check: {e}")
        return False


def connect_serial(port=None):
    com_port = port if port is not None else configs.com_port
    print(f"Connecting to port: {com_port}")
    try:
        if com_port.startswith('COM') or com_port.startswith('/dev/tty'):
            ser = serial.Serial(com_port, configs.com_port_baudrate, bytesize=8,
                                parity='N', stopbits=1, timeout=3, rtscts=False, dsrdtr=False)
            print(f"Serial port {com_port} opened successfully")
            return LoggedSerial(ser), {"test": "Serial Port", "status": "Success", "details": "Serial port opened successfully"}
        elif ':' in com_port:
            ser = serial.serial_for_url(
                f"socket://{com_port}", baudrate=configs.com_port_baudrate, timeout=3)
            print("Remote Serial on TCP port opened successfully")
            return LoggedSerial(ser), {"test": "TCP COM Port", "status": "Success", "details": "TCP  COM port opened successfully"}
        else:
            print(f"Invalid port format: {com_port}")
            return None, {"test": "Port", "status": "Failure", "details": f"Invalid port format: {com_port}"}
    except serial.SerialException as e:
        print(f"Failed to open port: {str(e)}")
        return None, {"test": "Port", "status": "Failure", "details": f"Failed to open port: {str(e)}"}
    except Exception as e:
        print(f"Unexpected error: {str(e)}")
        return None, {"test": "Port", "status": "Failure", "details": f"Unexpected error: {str(e)}"}


# Example call to the function for testing
if __name__ == "__main__":
    # Added to check if the script is being executed
    print("Starting the automation script")
    ser, result = connect_serial()
    print(result)
    ser.close()
