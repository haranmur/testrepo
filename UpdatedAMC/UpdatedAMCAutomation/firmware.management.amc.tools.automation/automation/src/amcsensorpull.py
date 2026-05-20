#Author: Haran Muru

import paramiko # type: ignore
import sensorparse
import serial_util
import sys

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
    
def clpeak(ip):
    test = runssh(ip, './Documents/AMCAutomation/clpeaktest.sh')
    return test

if __name__ == "__main__":
    sysip = "10.99.62.158"

    ser, result = serial_util.connect_serial("COM12")
    if ser is None:
        print(result["details"])
        sys.exit(1)

    clpeak(sysip)

    stats, filename = sensorparse.run_continuous_monitoring(ser, 43200, 5)
    print(f"\nAMC Sensor monitoring completed. Results saved to {filename}")

