#!/usr/bin/env python3

import subprocess
import re
from pathlib import Path

def get_pcie_widths():
    dgdiag_path = Path.home() / "DGDiagTools" / "DGDiagTool.exe"
    
    # Get actual width
    actual_result = subprocess.run(['sudo', str(dgdiag_path), '-PCIE.UTIL.GetLinkWidth'], 
                                 capture_output=True, text=True, cwd=str(dgdiag_path.parent))
    actual_match = re.search(r'x?(\d+)', actual_result.stdout)
    actual_width = int(actual_match.group(1)) if actual_match else None
    
    # Get max width
    max_result = subprocess.run(['sudo', str(dgdiag_path), '-PCIE.UTIL.GetMaxLinkWidth'], 
                              capture_output=True, text=True, cwd=str(dgdiag_path.parent))
    max_match = re.search(r'x?(\d+)', max_result.stdout)
    max_width = int(max_match.group(1)) if max_match else None
    
    print(f"Actual Width: x{actual_width}")
    print(f"Max Width: x{max_width}")

if __name__ == "__main__":
    get_pcie_widths()