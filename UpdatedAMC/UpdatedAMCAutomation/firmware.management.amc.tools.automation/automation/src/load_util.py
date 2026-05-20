import serial_util
import sys
import os
import time
import getopt
import sys
import configs  # Updated import
import re


# TODO: Add a check for 'kernel reboot' command not found
# TODO: Add a check if a wrong command is passed like "rest war"
# TODO: Add a check to find if the terminal is clean or some junk characters are already entered
# because this will make the test command issued fail if there is any junk characters or previously entered command is not executed.
amc_boot_log = r"\*\*\* Booting Loader! Version: \d+\.\d+\.\d+\.\d+ \*\*\*"
wdt_cmd_not_found = "watchdog: command not found"


def load_to_target(ser, filename):
    size = 0
    f = open(filename, 'rb')
    serial_util.set_serial_log_enabled(False)
    while True:
        piece = f.read(1024)
        if not piece:
            break
        ser.write(piece)
        size += 1024
    #    print("transfered {} bytes".format(size))
    serial_util.set_serial_log_enabled(True)
    f.close()


def find_bin_file(bin_folder, bin_file):

    # First check if the specified file exists
    bin_path = os.path.join(bin_folder, bin_file)
    if os.path.isfile(bin_path):
        print(f"✅ Found specified binary file: {bin_path}")
        return bin_path

    print(
        f"\n⚠️  Specified file {bin_path} not found, searching for uart_amc_*_dbg.bin pattern...")

    # Use os.listdir() to find matching files
    try:
        files = os.listdir(bin_folder)
        matching_files = []

        # Determine which pattern to search for based on bin_file name
        if re.match(r'^uart_amc.*_rel\.bin$', bin_file):
            pattern = r'^uart_amc.*_rel\.bin$'
        else:
            pattern = r'^uart_amc.*_dbg\.bin$'

        for file in files:
            if re.match(pattern, file):
                matching_files.append(os.path.join(bin_folder, file))

        if matching_files:
            # Sort files (newest first if timestamp in name)
            matching_files.sort(reverse=True)
            selected_file = matching_files[0]
            print(f"✅ Found wildcard match: {selected_file}")
            if len(matching_files) > 1:
                print(
                    f"ℹ️  Note: {len(matching_files)} files found, using: {os.path.basename(selected_file)}")
            return selected_file
    except OSError as e:
        raise Exception(f"❌ Cannot access folder {bin_folder}: {e}")

    # If no files found, raise exception
    raise Exception(
        f"❌ {bin_path} nor any uart_amc_*_dbg.bin pattern found in {bin_folder}")


def load_image(ser, reason):
    read_amc_boot_log = ""

    print(f"load image....{reason}")

    if reason == "reset cold" or reason == "reset warm" or reason == "watchdog feed_stop" or reason == "uart boot":
        if reason != "uart boot":
            ser.write(reason.encode())
            ser.write(configs.enter_1.encode())
            time.sleep(10)

        VAR_BIN_FILE = os.getenv('ENV_BIN_FILE', 'uart_amc.bin')
        BIN_FOLDER = os.getenv('ENV_BIN_FOLDER', '')

        while True:
            out = ser.read(4098)
            out_decoded = out.decode('utf-8', errors='replace')
            if wdt_cmd_not_found in out_decoded:
                raise Exception(
                    f"❌ {reason} Verification Failed : watchdog command not found")
            if len(out_decoded) > 0:
                print('------', end='')
                if out_decoded[-1] == 'U':
                    bin_path = find_bin_file(BIN_FOLDER, VAR_BIN_FILE)
                    print(f"Loading binary file: {bin_path}")
                    if not os.path.isfile(bin_path):
                        raise Exception(
                            f"❌ {reason} File not found: {bin_path} not found")
                    load_to_target(ser, bin_path)
                    time.sleep(60)
                    read_amc_boot_log = ser.read(10240)
                    break
                else:
                    print("Waiting for 'U' character to start loading binary file...")
                    time.sleep(1)

    print(f"Reading AMC boot log for {reason}...")
    if re.search(amc_boot_log, read_amc_boot_log.decode('utf-8')):
        print(f"✅ {reason} Boot string Verification Passed")
    else:
        print(f"❌ {reason}  Boot string  Verification Failed")
        print('error: ', read_amc_boot_log.decode('utf-8'))
        raise Exception(f"❌ {reason}  Boot string  Verification Failed")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)
    try:
        load_image(ser, "kernel reboot warm")
        time.sleep(1)
        load_image(ser, "watchdog feed_stop")
        time.sleep(1)
        load_image(ser, "kernel reboot cold")
    except Exception as e:
        print(str(e))
        sys.exit(1)
