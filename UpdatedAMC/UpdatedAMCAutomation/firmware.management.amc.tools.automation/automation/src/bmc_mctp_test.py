import configs
import subprocess
import time
import report_util
import os

BMC_IP = configs.bmc_ip
BMC_USER = configs.bmc_user
BMC_PASS = configs.bmc_password
PLDMTREE_CMD = "busctl tree xyz.openbmc_project.pldm"
BMCREBOOT_CMD = "/sbin/reboot"

# Log file setup with datetime suffix
try:
    dt_str = report_util.get_report_dt_str()
except Exception:
    dt_str = time.strftime("%Y%m%d_%H%M%S")
BMC_LOG_DIR = os.path.join(os.getcwd(), "results")
os.makedirs(BMC_LOG_DIR, exist_ok=True)
BMC_LOG_BASENAME = f"bmc_ssh_test_log_{dt_str}.txt"
BMC_LOG_FILENAME = os.path.join(BMC_LOG_DIR, BMC_LOG_BASENAME)

def log_bmc(msg):
    print(msg)
    with open(BMC_LOG_FILENAME, "a", encoding="utf-8") as f:
        f.write(msg + "\n")

def run_ssh_command(command, timeout=5, retries=6, wait_between=0):
    ssh_cmd = [
        "sshpass", "-p", BMC_PASS,
        "ssh", "-o", "StrictHostKeyChecking=no",
        f"{BMC_USER}@{BMC_IP}",
        command
    ]
    for attempt in range(retries):
        try:
            result = subprocess.run(
                ssh_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout
            )
            log_bmc(f"[SSH][{command}] stdout: {result.stdout.strip()} stderr: {result.stderr.strip()} rc: {result.returncode}")
            return result.stdout, result.stderr, result.returncode
        except subprocess.TimeoutExpired:
            log_bmc(f"SSH command timed out (attempt {attempt+1}/{retries}). Retrying in {wait_between}s...")
            time.sleep(wait_between)
    log_bmc(f"SSH command timed out after {retries} attempts: {command}")
    return "", f"SSH command timed out after {retries} attempts.", 1

def bmc_reboot():
    run_ssh_command(BMCREBOOT_CMD, timeout=60)
    log_bmc(f"BMC reboot done")
    return True

def wait_for_bmc_boot(timeout=300, interval=6):
    log_bmc("Waiting for BMC to boot and SSH to become available...")
    start_time = time.time()
    while (time.time() - start_time) < timeout:
        stdout, stderr, rc = run_ssh_command("uptime", timeout=10, retries=1, wait_between=interval)
        if rc == 0:
            log_bmc("BMC SSH is responsive.")
            return True
        else:
            log_bmc(f"BMC not up yet (rc={rc}). Retrying in {interval}s...")
            time.sleep(interval)
    log_bmc(f"BMC did not boot/respond to SSH within {timeout} seconds.")
    return False

def get_stable_bmctree_output(max_attempts=10, min_lines=30, wait_time=6):
    time.sleep(5)  # give time for mctp communication to stabilize
    for attempt in range(max_attempts):
        stdout, stderr, rc = run_ssh_command(PLDMTREE_CMD)
        if len(stdout.strip().splitlines()) > min_lines:
            return stdout, stderr, rc
        time.sleep(wait_time)
    return stdout, stderr, rc

def test_bmc_tree_contains_expected_paths():
    if not wait_for_bmc_boot():
        report_util.add_result("BMC SSH Boot", "Failure", "BMC did not boot/respond to SSH before reboot.")
        log_bmc("BMC did not boot/respond to SSH before reboot.")
        return False
    if not bmc_reboot(): # TODO : use PLDM daemon reboot command instead
        report_util.add_result("BMC SSH Reboot", "Failure", "BMC reboot command failed.")
        log_bmc("BMC reboot command failed.")
        return False
    if not wait_for_bmc_boot():
        report_util.add_result("BMC SSH Boot", "Failure", "BMC did not boot/respond to SSH after reboot.")
        log_bmc("BMC did not boot/respond to SSH after reboot.")
        return False
    stdout, stderr, rc = get_stable_bmctree_output()
    if rc != 0:
        report_util.add_result("BMC SSH BMCTREE", "Failure", f"SSH command failed: {stderr}")
        log_bmc(f"BMCTREE command failed: {stderr}")
        return False
    checks = [
        "/xyz/openbmc_project/pldm/1",
        "/xyz/openbmc_project/sensors/power",
        "/xyz/openbmc_project/sensors/current",
        "/xyz/openbmc_project/sensors/temperature",
        "/xyz/openbmc_project/sensors/voltage",
        "/xyz/openbmc_project/pldm/fru/1",
        "/xyz/openbmc_project/pldm/fwu/1",
    ]
    all_found = True
    for check in checks:
        if check in stdout:
            report_util.add_result("BMC mctp tree check", "Success", f"Found expected path: {check}")
            log_bmc(f"Found expected path: {check}")
        else:
            report_util.add_result("BMC mctp tree check", "Failure", f"Missing expected path: {check}")
            log_bmc(f"Missing expected path: {check}")
            all_found = False
    return all_found

def run_all_tests():
    log_bmc("Running BMC SSH tests...")
    print("Running BMC SSH tests...")
    result = test_bmc_tree_contains_expected_paths()
    if not result:
        log_bmc("BMC mctp probed path test failed.")
        print("BMC SSH test failed.")
        return False
    else:
        log_bmc("BMC mctp probed path test passed.")
        print("BMC SSH test passed.")

    # TODO : Add  more detail tests for fru/sensors etc

    return True

if __name__ == "__main__":
    # Only run if BMC_IP is set
    if BMC_IP:
        ok = run_all_tests()
        # Add log file link to HTML report
        report_util.add_result("BMC SSH Log", "Info", f"<a href='{BMC_LOG_BASENAME}' target='_blank'>BMC SSH Test Log</a>")
        exit(0 if ok else 1)
    else:
        print("BMC_IP is not set in environment/configs. Skipping BMC SSH tests.")
        report_util.add_result("BMC SSH Module", "Skipped", "BMC_IP is not set in environment/configs. Skipping BMC SSH tests.")
        exit(0)
