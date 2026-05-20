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

def warmr(ser):
    cmd = "warm reset"
    output = serial_util.send_command_and_read(ser, cmd, size=configs.size)

testComp = False

if __name__ == "__main__":
    sysip = "10.99.62.162"

    print("Enter Serial Port id spaced by commas (i.e. COM4, COM5) **Press enter to default to COM4")
    userPort = input()
    if userPort == "":
        userPort += 'COM4'
    portList = [word.strip().upper() for word in userPort.split(',')]

    print("\nChecking GPUs\n")
    preGPU = gpucheck(sysip)
    print(preGPU)



    for port in portList:
        if not port.startswith('COM'):
            print("False entry: " + port)
            sys.exit(1)
        else:
            print("\nStarting AMC warm reset for port: " + port + "\n")
            ser, result = serial_util.connect_serial(port)

            if ser is None:
                print(result["details"])
                sys.exit(1)
            try:
                warmr(ser)
                print("\nWaiting to finish WR test")
                time.sleep(5)
            
            except Exception as e:
                print(str(e))
                sys.exit(1)
            ser.close()

    print("\nChecking GPUs")
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
            testComp = False
        else:
            testComp = True
    else:      
        testComp = True

    if testComp == True:
        print("\nWR Test complete")
    else:
        print("\nWR Test did not complete")

    
