#Author: Haran Muru

import configs  # Updated import
import serial_util
import sys
import paramiko # type: ignore
import time

def runssh(ip, cmd):
    try:
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        ssh.connect(ip, username="gta", password="gta")

        stdin, stdout, stderr = ssh.exec_command(cmd)

        output = stdout.read().decode('utf-8', errors='replace').strip()
        error = stderr.read().decode('utf-8', errors='replace').strip()
        exit_status = stdout.channel.recv_exit_status()

        ssh.close()

        return output
    except Exception as e:
        print(f"SSH Error: {e}")

        return False
    
def gpucheck(ip):
    test = runssh(ip, 'python3 Documents/AMCAutomation/gpuchecker.py')
    return test
    
def unbind(ip):
    test = runssh(ip, 'python3 Documents/AMCAutomation/unbind_gpu_driver.py')
    return test

def rebind(ip):
    test = runssh(ip, 'python3 Documents/AMCAutomation/rescan.py')
    return test

def reset(ser, testType):
    if testType == "WARM":
        cmd = "reset warm"
    elif testType == "COLD":
        cmd = "reset cold"
    print("Running " + testType + " reset test.")
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

def run_reset_cycle(portList, testType, cycle_num):
    """Run a single reset cycle on all ports"""
    print(f"\n{'='*50}")
    print(f"Starting Reset Cycle {cycle_num}")
    print(f"{'='*50}")
    
    for port in portList:
        if not port.startswith('COM'):
            print("False entry: " + port)
            return False
        else:
            print(f"\nStarting AMC reset for port: {port} (Cycle {cycle_num})")
            ser, result = serial_util.connect_serial(port)

            if ser is None:
                print(result["details"])
                return False
            try:
                reset(ser, testType)
                print("\nWaiting to finish test")
                time.sleep(5)
            
            except Exception as e:
                print(str(e))
                return False
            finally:
                ser.close()
    
    return True

if __name__ == "__main__":
    sysip = "10.99.62.158"

    print("Enter test type: warm or cold?")
    testType = input()
    testType = testType.strip().upper()
    if testType != "WARM" and testType != "COLD":
        print("Improper test input. Default to warm")
        testType = "WARM"

    print("Enter Serial Port id spaced by commas (i.e. COM4, COM5) **Press enter to default to all COM ports")
    userPort = input()
    if userPort == "":
        portList = serial_util.listPorts()
    else:
        portList = [word.strip().upper() for word in userPort.split(',')]

    print("Enter number of cycles to run (Press enter for 1 cycle):")
    cycles_input = input().strip()
    if cycles_input == "":
        num_cycles = 1
    else:
        try:
            num_cycles = int(cycles_input)
            if num_cycles <= 0:
                print("Invalid number of cycles. Defaulting to 1.")
                num_cycles = 1
        except ValueError:
            print("Invalid input. Defaulting to 1 cycle.")
            num_cycles = 1

    print(f"\nWill run {num_cycles} cycle(s) of {testType} reset test")
    print(f"Ports to test: {', '.join(portList)}")

    # Run the specified number of cycles
    all_cycles_successful = True
    
    for cycle in range(1, num_cycles + 1):
        print("\nChecking GPUs before starting cycle\n")
        preGPU = gpucheck(sysip)
        print(preGPU)
        
        cycle_success = run_reset_cycle(portList, testType, cycle)
        
        print("\nChecking GPUs after all cycles")
        postGPU = gpucheck(sysip)
        print(postGPU)
        
        if preGPU != postGPU:
            print("\nOne or more GPUs are missing after the reset test")
            print("\nUnbinding and rebinding the drivers to check")
            runUnbind = unbind(sysip)
            time.sleep(10)
            runRebind = rebind(sysip)
            time.sleep(10)

            gpuRecheck = gpucheck(sysip)

            if gpuRecheck != preGPU:
                cycle_success = False
            else:
                cycle_success = True
        else:      
            cycle_success = True

        if not cycle_success:
            print(f"\nCycle {cycle} failed!")
            all_cycles_successful = False
            break
        
        print(f"\nCycle {cycle} completed successfully")

        # Add pause between cycles (except after the last cycle)
        if cycle < num_cycles:
            print(f"\nPausing for 15 seconds before next cycle...")
            time.sleep(15)

    if not all_cycles_successful:
        print(f"\nTest stopped due to failure in cycle {cycle}")
        sys.exit(1)

    print(f"\nAll {num_cycles} cycle(s) completed successfully!")