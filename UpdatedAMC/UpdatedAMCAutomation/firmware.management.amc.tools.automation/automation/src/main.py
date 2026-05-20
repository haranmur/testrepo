import kvm_pdu_util
import bmc_mctp_test
import gpio_util
import i3c
import version_amc
import zephyar_default
import timers
import spi
import eeprom
import fru
import i2c
import report_util
import sensors
import reset
import watchdog
import configs  # Updated import
import power_sequence
import time
import serial_util
import sys
import os
import pmt
import peci

# Add the src directory to the system path
sys.path.append(os.path.join(os.path.dirname(__file__), 'src'))


# Attempt to open the serial port
ser, result = serial_util.connect_serial()
report_util.add_result(result["test"], result["status"], result["details"])

if not ser:
    report_util.generate_html_report()
    sys.exit(1)

pdu_ip = configs.raritan_pdu_ip

if pdu_ip:
    # Reset KVM PDU before any other operations
    try:
        kvm_pdu_util.reset_kvm_pdu()
    except Exception as e:
        report_util.add_result("KVM PDU Reset", "Failure", str(e))
        print(f"Exception: {e}")
    time.sleep(2)
    try:
        reset.uart_boot(ser)
    except Exception as e:
        report_util.add_result("Uart Boot", "Failure", str(e))
        print(f"Exception: {e}")
    time.sleep(1)
else:
    print("KVM PDU IP", "Skip",
          "PDU IP is not configured, skipping KVM PDU reset")


# Run BMC SSH module test if bmc_ip is set
if configs.bmc_ip:
    try:
        bmc_result = bmc_mctp_test.run_all_tests()
        if bmc_result:
            print("BMC SSH Module", "Success",
                 "All BMC SSH test cases passed.")
        else:
            print("BMC SSH Module", "Failure",
                 "Some BMC SSH test cases failed.")
    except Exception as e:
        # report_util.add_result("BMC SSH Module", "Failure", f"Exception in BMC SSH module: {e}")
        print(f"Exception in BMC SSH module: {e}")
else:
    print("BMC SSH Module", "Skipped",
         "BMC_IP is not set, skipping BMC SSH tests.")

# Check for AMC prompt before running any tests
print("Checking for AMC prompt...")
amc_shell_present = serial_util.check_amc_prompt(ser)

# Run all other test modules only if AMC shell is present
if amc_shell_present:
    for mod, name in [
        #    (power_sequence, "Power Sequence"),
        (i3c, "I3C"),
        # (watchdog, "Watchdog"),
        (sensors, "Sensors"),
        (i2c, "I2C"),
        (fru, "FRU"),
        (eeprom, "EEPROM"),
        (spi, "SPI"),
        #(timers, "Timers"),
        (zephyar_default, "Zephyar Default"),
        (version_amc, "AMC Version"),
        (gpio_util, "GPIO"),
        (pmt, "PMT"),
        (peci, "PECI"),
        # Add more as needed (e.g., (i3c, "I3C"))
    ]:
        try:
            mod.run_all_tests(ser)
    #        report_util.add_result(f"{name} Module", "Success", f"All test case passed in {name} module")
        except Exception as e:
            report_util.add_result(
                f"{name} Module", "Failure", f"Exception in {name} module: {e}")
            print(f"Exception in {name} module: {e}")
else:
    print("❌ AMC shell not present. Skipping test modules.")
    report_util.add_result("Test Modules", "Skipped",
                          "AMC shell not present, test modules skipped.")

# Generate HTML report
report_util.generate_html_report()

ser.close()
# Exit if there are any failures
if any(result["status"] == "Failure" for result in report_util.results):
    sys.exit(1)
