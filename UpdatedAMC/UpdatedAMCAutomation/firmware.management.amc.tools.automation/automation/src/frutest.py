import configs  # Updated import
import serial_util
import sys
import report_util
import time

def setFruSerial(ser, customSerial):
    cmd = "fru set_serial_number " + customSerial    
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

def amcReset(ser):
    cmd = "reset warm"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

def bmcLogin(ser):
    output = serial_util.send_command_and_read(ser, "root", size=configs.size)
    output = serial_util.send_command_and_read(ser, "0penBmc1", size=configs.size)

def bmcReboot(ser):
    cmd = "reboot"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

def bmcFruPull(ser):
    cmd = "busctl introspect xyz.openbmc_project.pldm /xyz/openbmc_project/FruDevice/inteldatacentergpuflexseries_1"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)
    return output

def parse_and_compare_serial(bmc_output, test_string):
    """Parse serial number from BMC output and compare to test string"""
    lines = bmc_output.split('\n')
    
    for line in lines:
        if 'PRODUCT_SERIAL_NUMBER' in line:
            # Extract the serial number between quotes
            start = line.find('"') + 1
            end = line.rfind('"')
            if start > 0 and end > start:
                bmc_serial = line[start:end].strip()
                print()
                print(f"Set Serial: {test_string}")
                print(f"BMC Serial: {bmc_serial}")
                print()

                if test_string == bmc_serial:
                    print("PASS: Serial numbers match")
                    return True
                else:
                    print("FAIL: Serial numbers do not match")
                    return False
    
    print("FAIL: Could not find serial number in BMC output")
    return False

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
        print("\nStarting FRU test for port: " + amcPort + "\n")
        ser, result = serial_util.connect_serial(amcPort)

        if ser is None:
            print(result["details"])
            sys.exit(1)
        try:
            testString = str(int(time.time()))
            print("\nTest string for serial number: " + testString)
            
            setFruSerial(ser, testString)

            print("\nRebooting AMC")
            amcReset(ser)          
            time.sleep(25)
        
        except Exception as e:
            print(str(e))
            sys.exit(1)
        ser.close()

        serbmc, resbmc = serial_util.connect_serial(bmcPort)
        if serbmc is None:
            print(resbmc["details"])
            sys.exit(1)
        try:
            print("\nRebooting BMC")
            bmcReboot(serbmc)
            time.sleep(65)

            bmcLogin(serbmc)

            time.sleep(10)

            bmc_output = bmcFruPull(serbmc)
            print(bmc_output)
            
            test_passed = parse_and_compare_serial(bmc_output, testString)
        
        except Exception as e:
            print(str(e))
            sys.exit(1)
        serbmc.close()