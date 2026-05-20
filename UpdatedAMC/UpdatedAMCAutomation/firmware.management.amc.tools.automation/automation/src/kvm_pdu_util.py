import time
import pexpect
import configs

def reset():
    hostname = configs.raritan_pdu_ip
    username = configs.raritan_pdu_username
    password = configs.raritan_pdu_password
    sut_outletIndex = configs.raritan_pdu_outletIndex

    print (f"Resetting KVM PDU at {username}@{hostname} for outlet index {sut_outletIndex}...")
    try:
        child = pexpect.spawn(f'ssh {username}@{hostname}')
        i = child.expect(['password:', 'Are you sure you want to continue connecting (yes/no/[fingerprint])?'])
        if i == 1:
            child.sendline('yes')
            child.expect('password:')
        child.sendline(password)
        child.expect(r'\[.*\] >')
        command = f'power outlets {sut_outletIndex} cycle /y'
        child.sendline(command)
        child.expect(r'\[.*\] >')

        # Wait for the outlet to turn ON
        print(f"Waiting for outlet {sut_outletIndex} to turn ON ...")
        max_attempts = 15
        for attempt in range(max_attempts):
            time.sleep(8)
            show_cmd = f'show outlets {sut_outletIndex}'
            child.sendline(show_cmd)
            child.expect(r'\[.*\] >')
            output = child.before.decode(errors='ignore') if hasattr(child.before, 'decode') else str(child.before)
            ##print(output)
            if 'Power state: On' in output:
                print(f"Outlet {sut_outletIndex} is ON.")
                break
        else:
            print(f"Timeout waiting for outlet {sut_outletIndex} to turn ON.")

        child.sendline('exit')
        child.expect(pexpect.EOF)
        child.close()

        if child.exitstatus is not None and child.exitstatus != 0:
            print(f"Failed to reset KVM PDU [exit status: {child.exitstatus}]")
            raise Exception("Failed to reset KVM PDU")
        print("KVM PDU reset command sent successfully")
    except Exception as e:
        raise Exception(f"Failed to reset KVM PDU: {str(e)}")

def reset_kvm_pdu():
    try:
        reset()
        print("✅ KVM PDU Reset Successful")
    except Exception as e:
        print(f"❌ KVM PDU Reset Failed: {str(e)}")
        raise

if __name__ == "__main__":
    reset_kvm_pdu()
