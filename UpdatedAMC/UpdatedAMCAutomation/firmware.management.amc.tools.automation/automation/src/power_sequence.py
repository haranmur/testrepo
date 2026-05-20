import serial_util
import sys
import os
import time
import re
import configs

repeated_pwr_dwn_cmd_patterns = [
    r"Powering down : 0",
    r"System is not powered up"
]


repeated_pwr_up_cmd_patterns = [
    r"Powering up : 1",
    r"System is already powered up"
]

found_repeated_pwr_up_patterns = [False] * len(repeated_pwr_up_cmd_patterns)

power_up_patterns = [
    r"<inf> AMC_POWER: Board status wait host power ok",
    r"<inf> AMC_POWER: Product Name : mvs75",
    r"<inf> AMC_POWER: power up sequence",
    r"<inf> AMC_POWER: ALL Pin up is high",
    r"<inf> AMC_POWER: VR_GPU1_EN is set to high",
    r"<inf> AMC_POWER: VCCGT_GPU1_PWRGD is high",
    r"<inf> AMC_POWER: P1V8_VR_EN is set to high",
    r"<inf> AMC_POWER: P1V8_GPU1_VR_PWRGD is high",
    r"<inf> AMC_POWER: P1V2_GPU1_VR_EN is set to high",
    r"<inf> AMC_POWER: P1V2_GPU1_VR_PWRGD is high"
]

power_down_patterns = [
    r"<inf> AMC_POWER: Power down sequence started",
    r"<inf> AMC_POWER: P1V2_GPU1_VR_EN is set to low",
    r"<inf> AMC_POWER: P1V8_GPU1_VR_EN is set to low",
    r"<inf> AMC_POWER: VR_GPU1_EN is set to low",
    r"<inf> AMC_POWER: Power down status = 6", # TODO: should we keep this hard coded value, will this value change based on the board type
    r"<inf> AMC_POWER: Board status wait power up",
]

def power_up(ser):
    print("##########################################")
    print("Power Up only")
    cmd="power up"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

def power_down(ser):
    print("##########################################")
    print("Power down only")
    cmd="power down"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

def power_down_sequence_test(ser):
    found_power_dwn_patterns = [False] * len(power_down_patterns)
    print("##########################################")
    print("Power Down Sequence")
    cmd="power down"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

    for i, pattern in enumerate(power_down_patterns):
        if not found_power_dwn_patterns[i] and re.search(pattern, output):
            print(f"Found match for power down : {pattern}")
            found_power_dwn_patterns[i] = True

    if all(found_power_dwn_patterns):
        print("✅ Success: Power Down test!")
    else:
        print("❌ Failure: Power-Down patterns not found")
        raise Exception("❌ Failure: Power-Down patterns not found, power sequencing might be wrong, 'power up' first and test again !!!")

def power_up_sequence_test(ser):
    # List of strings to search for
    found_power_up_patterns = [False] * len(power_up_patterns)
    print("##########################################")
    print("Power Up Sequence")
    cmd="power up"
    #Read command’s output
    output = serial_util.send_command_and_read(ser, cmd, timeout=5)

    for i, pattern in enumerate(power_up_patterns):
        if not found_power_up_patterns[i] and re.search(pattern, output):
            print(f"Found match : {pattern}")
            found_power_up_patterns[i] = True

    if all(found_power_up_patterns):
        print("✅ Success: Power Up test!")
    else:
        raise Exception("❌ Failure: Power-Up patterns not found, power sequencing might be wrong, 'power down' first and test again !!!")

def check_previous_power_cmd(ser):
    found_power_dwn_patterns = [False] * len(power_down_patterns)
    found_repeated_pwr_dwn_patterns = [False] * len(repeated_pwr_dwn_cmd_patterns)

    print("check previous power cmd")
    cmd="power down"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    print(output)

    #Check for power down command output matching
    for i, pattern in enumerate(power_down_patterns):
        if not found_power_dwn_patterns[i] and re.search(pattern, output):
            print(f"Found match for power down : {pattern}")
            found_power_dwn_patterns[i] = True

    if all(found_power_dwn_patterns):
        print("All Power-Down patterns detected!")
        power_up(ser)
        time.sleep(1)
        print("The previous command executed on amc terminal was \"power up\"")
        return "power up"

    #Check for power down command repetition output matching
    for i, pattern in enumerate(repeated_pwr_dwn_cmd_patterns):
        if not found_repeated_pwr_dwn_patterns[i] and re.search(pattern, output):
            print(f"Found match for repeated power down cmd : {pattern}")
            found_repeated_pwr_dwn_patterns[i] = True

    if all(found_repeated_pwr_dwn_patterns):
        print("Repeated Power-Down patterns detected!")
        print("The previous command executed on amc terminal was \"power down\"")
        return "power down"
    else:
        raise Exception("❌ Failure: power on/off sequence cann't decide the previous command")



def run_all_tests(ser):
    import report_util

    try:
        previous_cmd = check_previous_power_cmd(ser)
        if (previous_cmd == "power up"):
            power_down_sequence_test(ser)
            power_up_sequence_test(ser)
        elif (previous_cmd == "power down"):
            power_up_sequence_test(ser)
            power_down_sequence_test(ser)
        elif (previous_cmd == "command not found"):
            print("Command not found")
        report_util.add_result("Power Sequence", "Success", "Power Sequence verification passed")
    except Exception as e:
        report_util.add_result("Power Sequence", "Failure", str(e))
        print(f"Exception: {e}")


if __name__ == "__main__":
    ser, result = serial_util.connect_serial()
    if ser is None:
        print(result["details"])
        sys.exit(1)

    run_all_tests(ser)
    ser.close()
    print("All Power Sequence tests completed.")