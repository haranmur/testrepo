#!/usr/bin/env python3
"""
Automated Host Testing Script

This script performs:
- PCIe slot testing using DGDiag (Get vs MaxGet comparison)
- CPU benchmarking with mprime for set minutes
- Real-time monitoring of CPU speeds and temperatures
- Enhanced temperature monitoring: AMD (Tctl + Tccd), Intel (all CPU temps)
- Simple text file output with results

Author: Haran Muru
"""

import subprocess
import logging
import os
import sys
import time
import platform
import psutil
import threading
import signal
import glob
import re
from datetime import datetime
from pathlib import Path
import argparse

class HostTester:
    def __init__(self, logPath="./host_tests"):
        self.logPath = Path(logPath)
        self.monitoringActive = False
        self.monitoringData = {
            'cpuSpeeds': [],
            'temperatures': [],
            'timestamps': []
        }
        
        # Fixed benchmark duration
        self.benchmarkDurationMinutes = 3
        
        # Define tool paths
        self.dgDiagPath = Path.home() / "DGDiagTool_Internal"
        self.mprimePath = Path.home() / "Documents" / "hostTestScripts" / "p95v3019b20.linux64"
        
        # Create log directory
        self.logPath.mkdir(parents=True, exist_ok=True)
        
        # Setup logging
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.logFile = self.logPath / f"{timestamp}_{self.getMoboName()}.log"
        
        # Create hostname-based output file
        hostname = platform.node().replace('.', '_').replace('-', '_')
        self.outputFile = self.logPath / f"{timestamp}_{self.getMoboName()}_test_results.txt"

        self.setupLogging()
        
        # Detect CPU vendor
        self.cpuVendor = self.detectCpuVendor()
        
        # Verify tool paths
        self.verifyToolPaths()
        
        # Test results
        self.testResults = {
            "hostname": platform.node(),
            "testDate": datetime.now().isoformat(),
            "cpuVendor": self.cpuVendor,
            "pcieTestStatus": "NOT_RUN",
            "cpuBenchmarkCompleted": False,
            "monitoringSummary": {}
        }
        
    def setupLogging(self):
        """Setup logging configuration"""
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s [%(levelname)s] %(message)s',
            handlers=[
                logging.FileHandler(self.logFile),
                logging.StreamHandler(sys.stdout)
            ]
        )
        self.logger = logging.getLogger(__name__)
       
    def getMoboName(self):
        try:
            command = "sudo dmidecode -s baseboard-product-name"
            result = subprocess.run(command, shell=True, capture_output=True, text=True, check=True)
            return result.stdout.strip()
        except:
            return 'unknown'

    def verifyToolPaths(self):
        """Verify that required tools exist at specified paths"""
        # Check DGDiagTool
        dgDiagExe = self.dgDiagPath / "DGDiagTool"
        if not dgDiagExe.exists():
            self.logger.error(f"DGDiagTool not found at: {dgDiagExe}")
            self.logger.error(f"Expected path: {self.dgDiagPath.absolute()}")
        else:
            self.logger.info(f"Found DGDiagTool at: {dgDiagExe.absolute()}")
        
        # Check mprime
        mprimeExe = self.mprimePath / "mprime"
        if not mprimeExe.exists():
            self.logger.warning(f"mprime not found at: {mprimeExe}")
            self.logger.warning(f"Expected path: {self.mprimePath.absolute()}")
            self.logger.info("Will attempt to use stress-ng as fallback")
        else:
            self.logger.info(f"Found mprime at: {mprimeExe.absolute()}")

    def detectCpuVendor(self):
        """Detect CPU vendor (Intel or AMD)"""
        try:
            with open('/proc/cpuinfo', 'r') as f:
                cpuInfo = f.read()
            
            if 'GenuineIntel' in cpuInfo:
                return 'intel'
            elif 'AuthenticAMD' in cpuInfo:
                return 'amd'
            else:
                return 'unknown'
        except:
            return 'unknown'
    
    def runDgDiag(self, command, timeout=30):
        """Execute DG diagnostic command from DGDiagTools_Internal directory"""
        dgDiagExe = self.dgDiagPath / "DGDiagTool"
        
        if not dgDiagExe.exists():
            errorMsg = f"DGDiagTool not found at: {dgDiagExe.absolute()}"
            self.logger.error(errorMsg)
            return {
                "success": False,
                "output": "",
                "error": errorMsg,
                "exitCode": -1,
                "command": f"{dgDiagExe} {command}"
            }
        
        fullCommand = ['sudo', str(dgDiagExe), command]
        
        try:
            self.logger.info(f"Executing: {' '.join(fullCommand)} (from {self.dgDiagPath.absolute()})")
            
            result = subprocess.run(
                fullCommand,
                cwd=str(self.dgDiagPath.absolute()),  # Run from DGDiagTools_Internal directory
                capture_output=True,
                text=True,
                timeout=timeout
            )
            
            return {
                "success": result.returncode == 0,
                "output": result.stdout.strip(),
                "error": result.stderr.strip(),
                "exitCode": result.returncode,
                "command": ' '.join(fullCommand)
            }
            
        except subprocess.TimeoutExpired:
            errorMsg = f"Command timed out after {timeout} seconds"
            self.logger.error(errorMsg)
            return {
                "success": False,
                "output": "",
                "error": errorMsg,
                "exitCode": -1,
                "command": ' '.join(fullCommand)
            }
        except Exception as e:
            errorMsg = f"Error executing DG diagnostic command: {str(e)}"
            self.logger.error(errorMsg)
            return {
                "success": False,
                "output": "",
                "error": errorMsg,
                "exitCode": -1,
                "command": ' '.join(fullCommand)
            }
    
    def getAllCpuTemperatures(self):
        """Get all CPU temperatures based on vendor"""
        try:
            if self.cpuVendor == 'intel':
                return self.getAllIntelTemps()
            elif self.cpuVendor == 'amd':
                return self.getAllAmdTemps()
            else:
                return self.getGenericTemps()
        except Exception as e:
            self.logger.warning(f"Could not read CPU temperatures: {e}")
            return {}
    
    def getAllIntelTemps(self):
        """Get all Intel CPU temperatures using lm-sensors"""
        temperatures = {}
        
        try:
            result = subprocess.run(['sensors'], capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                currentSensor = None
                for line in result.stdout.split('\n'):
                    line = line.strip()
                    
                    # Detect Intel temperature sensors
                    if any(sensor in line.lower() for sensor in ['coretemp', 'k10temp', 'acpi']):
                        currentSensor = line
                        continue
                    
                    # Parse temperature lines
                    if currentSensor and ':' in line and '°C' in line:
                        # Look for CPU-related temperature readings
                        if any(keyword in line.lower() for keyword in ['core', 'package', 'cpu', 'temp']):
                            match = re.search(r'([^:]+):\s*\+?(\d+\.?\d*)°C', line)
                            if match:
                                tempName = match.group(1).strip()
                                tempValue = float(match.group(2))
                                temperatures[tempName] = tempValue
                
                # If no specific sensors found, try direct parsing
                if not temperatures:
                    for line in result.stdout.split('\n'):
                        if any(keyword in line.lower() for keyword in ['core', 'package']) and '°C' in line:
                            match = re.search(r'([^:]+):\s*\+?(\d+\.?\d*)°C', line)
                            if match:
                                tempName = match.group(1).strip()
                                tempValue = float(match.group(2))
                                temperatures[tempName] = tempValue
                                
        except Exception as e:
            self.logger.warning(f"Error reading Intel temperatures with sensors: {e}")
        
        # Fallback to hwmon if sensors didn't work
        if not temperatures:
            temperatures = self.getHwmonTemperatures()
        
        return temperatures
    
    def getAllAmdTemps(self):
        """Get AMD CPU temperatures (Tctl and Tccd) using k10temp"""
        temperatures = {}
        
        try:
            result = subprocess.run(['sensors'], capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                inK10temp = False
                for line in result.stdout.split('\n'):
                    line = line.strip()
                    
                    # Detect k10temp sensor section
                    if 'k10temp' in line.lower():
                        inK10temp = True
                        continue
                    
                    # Reset when we hit a new sensor section
                    if line and not line.startswith(' ') and ':' not in line and inK10temp:
                        if any(char.isalpha() for char in line) and 'k10temp' not in line.lower():
                            inK10temp = False
                    
                    # Parse temperature lines in k10temp section
                    if inK10temp and ':' in line and '°C' in line:
                        # Look for Tctl and Tccd temperatures
                        if 'tctl' in line.lower() or 'tccd' in line.lower():
                            match = re.search(r'([^:]+):\s*\+?(\d+\.?\d*)°C', line)
                            if match:
                                tempName = match.group(1).strip()
                                tempValue = float(match.group(2))
                                temperatures[tempName] = tempValue
                
        except Exception as e:
            self.logger.warning(f"Error reading AMD temperatures with sensors: {e}")
        
        # Fallback to direct hwmon reading for k10temp
        if not temperatures:
            try:
                hwmonPaths = glob.glob('/sys/class/hwmon/hwmon*')
                for path in hwmonPaths:
                    nameFile = os.path.join(path, 'name')
                    if os.path.exists(nameFile):
                        with open(nameFile, 'r') as f:
                            if f.read().strip() == 'k10temp':
                                # Read all temperature inputs for k10temp
                                tempFiles = glob.glob(os.path.join(path, 'temp*_input'))
                                for tempFile in tempFiles:
                                    try:
                                        with open(tempFile, 'r') as f:
                                            tempVal = int(f.read().strip()) / 1000.0
                                        
                                        # Get label if available
                                        tempNum = re.search(r'temp(\d+)_input', tempFile).group(1)
                                        labelFile = os.path.join(path, f'temp{tempNum}_label')
                                        
                                        if os.path.exists(labelFile):
                                            with open(labelFile, 'r') as f:
                                                label = f.read().strip()
                                            temperatures[label] = tempVal
                                        else:
                                            temperatures[f'temp{tempNum}'] = tempVal
                                    except Exception as e:
                                        continue
                                break
            except Exception as e:
                self.logger.warning(f"Error reading AMD temperatures from hwmon: {e}")
        
        return temperatures
    
    def getHwmonTemperatures(self):
        """Get temperatures from hwmon interface"""
        temperatures = {}
        
        try:
            hwmonPaths = glob.glob('/sys/class/hwmon/hwmon*')
            for path in hwmonPaths:
                nameFile = os.path.join(path, 'name')
                if os.path.exists(nameFile):
                    with open(nameFile, 'r') as f:
                        sensorName = f.read().strip()
                    
                    # Look for CPU-related sensors
                    if any(keyword in sensorName.lower() for keyword in ['coretemp', 'k10temp', 'cpu']):
                        tempFiles = glob.glob(os.path.join(path, 'temp*_input'))
                        for tempFile in tempFiles:
                            try:
                                with open(tempFile, 'r') as f:
                                    tempVal = int(f.read().strip()) / 1000.0
                                
                                # Get label if available
                                tempNum = re.search(r'temp(\d+)_input', tempFile).group(1)
                                labelFile = os.path.join(path, f'temp{tempNum}_label')
                                
                                if os.path.exists(labelFile):
                                    with open(labelFile, 'r') as f:
                                        label = f.read().strip()
                                    temperatures[f"{sensorName}_{label}"] = tempVal
                                else:
                                    temperatures[f"{sensorName}_temp{tempNum}"] = tempVal
                            except Exception as e:
                                continue
        except Exception as e:
            self.logger.warning(f"Error reading hwmon temperatures: {e}")
        
        return temperatures
    
    def getGenericTemps(self):
        """Get temperature using generic thermal zone"""
        temperatures = {}
        
        try:
            thermalZones = glob.glob('/sys/class/thermal/thermal_zone*')
            for i, zone in enumerate(thermalZones):
                tempFile = os.path.join(zone, 'temp')
                typeFile = os.path.join(zone, 'type')
                
                if os.path.exists(tempFile):
                    with open(tempFile, 'r') as f:
                        temp = int(f.read().strip()) / 1000.0
                    
                    # Get zone type if available
                    zoneType = f"thermal_zone{i}"
                    if os.path.exists(typeFile):
                        try:
                            with open(typeFile, 'r') as f:
                                zoneType = f.read().strip()
                        except:
                            pass
                    
                    temperatures[zoneType] = temp
        except Exception as e:
            self.logger.warning(f"Error reading generic temperatures: {e}")
        
        return temperatures
    
    def getCpuFrequencies(self):
        """Get current CPU frequencies"""
        try:
            # Try psutil first
            freqs = psutil.cpu_freq(percpu=True)
            if freqs:
                return [freq.current for freq in freqs]
        except:
            pass
        
        # Fallback to /proc/cpuinfo
        try:
            with open('/proc/cpuinfo', 'r') as f:
                content = f.read()
            
            freqs = []
            for line in content.split('\n'):
                if 'cpu MHz' in line:
                    freq = float(line.split(':')[1].strip())
                    freqs.append(freq)
            return freqs
        except:
            pass
        
        return []
    
    def startMonitoring(self):
        """Start background monitoring of CPU speeds and temperatures"""
        self.monitoringActive = True
        self.monitoringThread = threading.Thread(target=self.monitoringLoop)
        self.monitoringThread.daemon = True
        self.monitoringThread.start()
        self.logger.info("Started background monitoring")
    
    def stopMonitoring(self):
        """Stop background monitoring"""
        self.monitoringActive = False
        if hasattr(self, 'monitoringThread'):
            self.monitoringThread.join(timeout=5)
        self.logger.info("Stopped background monitoring")
    
    def monitoringLoop(self):
        """Background monitoring loop"""
        while self.monitoringActive:
            try:
                timestamp = time.time()
                
                # Get CPU frequencies
                freqs = self.getCpuFrequencies()
                if freqs:
                    avgFreq = sum(freqs) / len(freqs)
                    maxFreq = max(freqs)
                    minFreq = min(freqs)
                    
                    self.monitoringData['cpuSpeeds'].append({
                        'timestamp': timestamp,
                        'average': avgFreq,
                        'maximum': maxFreq,
                        'minimum': minFreq,
                        'allCores': freqs
                    })
                
                # Get all CPU temperatures
                temps = self.getAllCpuTemperatures()
                if temps:
                    tempReading = {
                        'timestamp': timestamp,
                        'temperatures': temps
                    }
                    self.monitoringData['temperatures'].append(tempReading)
                
                self.monitoringData['timestamps'].append(timestamp)
                
                time.sleep(1)  # Monitor every second
                
            except Exception as e:
                time.sleep(1)
    
    def runCpuBenchmark(self):
        """Run CPU benchmark with monitoring for 10 minutes"""
        self.logger.info(f"Starting CPU benchmark for {self.benchmarkDurationMinutes} minutes")
        
        # Check if mprime exists at specified path
        mprimeExe = self.mprimePath / "mprime"
        if not mprimeExe.exists():
            self.logger.warning(f"mprime not found at: {mprimeExe.absolute()}")
            self.logger.warning("Using stress-ng as fallback")
            return self.runStressNgFallback()
        
        try:
            # Start mprime process from the specified directory
            mprimeCmd = [str(mprimeExe), '-t']  # Torture test mode
            
            self.logger.info(f"Starting mprime from: {self.mprimePath.absolute()}")
            self.logger.info(f"Command: {' '.join(mprimeCmd)}")
            
            process = subprocess.Popen(
                mprimeCmd,
                cwd=str(self.mprimePath.absolute()),  # Run from mprime directory
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            self.logger.info(f"Started mprime benchmark with PID {process.pid}")
            
            # Start monitoring
            self.startMonitoring()
            
            # Wait for set minutes
            time.sleep(self.benchmarkDurationMinutes * 60)
            
            # Stop mprime gracefully
            self.logger.info("Stopping mprime benchmark...")
            try:
                process.send_signal(signal.SIGINT)
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.logger.warning("mprime didn't stop gracefully, force killing...")
                process.kill()
                process.wait()
            
            # Stop monitoring
            self.stopMonitoring()
            
            self.logger.info("CPU benchmark completed")
            return True
            
        except Exception as e:
            self.logger.error(f"Error running CPU benchmark: {str(e)}")
            self.stopMonitoring()
            return False
    
    def runStressNgFallback(self):
        """Fallback benchmark using stress-ng for set minutes"""
        self.logger.info(f"Running stress-ng CPU benchmark for {self.benchmarkDurationMinutes} minutes")
        
        try:
            cpuCount = psutil.cpu_count()
            stressCmd = [
                'stress-ng',
                '--cpu', str(cpuCount),
                '--timeout', f'{self.benchmarkDurationMinutes}m',
                '--metrics-brief'
            ]
            
            # Start monitoring
            self.startMonitoring()
            
            result = subprocess.run(stressCmd, capture_output=True, text=True, 
                                  timeout=self.benchmarkDurationMinutes*60+30)
            
            # Stop monitoring
            self.stopMonitoring()
            
            if result.returncode == 0:
                self.logger.info("stress-ng benchmark completed successfully")
                return True
            else:
                self.logger.error(f"stress-ng failed: {result.stderr}")
                return False
                
        except Exception as e:
            self.logger.error(f"Error running stress-ng: {str(e)}")
            self.stopMonitoring()
            return False
    
    def testPcieSlots(self):
        """Test PCIe Slots using DGDiag - Compare Get (actual) vs MaxGet (expected)"""
        self.logger.info("=== PCIe Slots Test ===")
        self.logger.info("Comparing current values (Get) with maximum capabilities (MaxGet)")
        
        overallStatus = "PASS"
        pcieResults = {
            "actualSpeed": None,
            "expectedSpeed": None,
            "actualWidth": None,
            "expectedWidth": None,
            "speedMatch": False,
            "widthMatch": False
        }
        
        try:
            # Get current link speed (actual)
            self.logger.info("Getting current PCIe link speed...")
            speedResult = self.runDgDiag("-PCIE.UTIL.GetLinkSpeed")
            if speedResult["success"]:
                try:
                    speedMatch = re.search(r'Current Link Speed\s*:\s*Gen(\d+)', speedResult["output"])
                    if speedMatch:
                        pcieResults["actualSpeed"] = int(speedMatch.group(1))
                        self.logger.info(f"Current Link Speed: gen {pcieResults['actualSpeed']}")
                except (ValueError, AttributeError):
                    self.logger.warning(f"Could not parse current speed: {speedResult['output']}")
            else:
                self.logger.error(f"Failed to get current link speed: {speedResult['error']}")
            
            # Get maximum link speed (expected)
            self.logger.info("Getting maximum PCIe link speed...")
            maxSpeedResult = self.runDgDiag("-PCIE.UTIL.GetMaxLinkSpeed")
            if maxSpeedResult["success"]:
                try:
                    speedMatch = re.search(r'Max Link Speed\s*:\s*Gen(\d+)', maxSpeedResult["output"])
                    if speedMatch:
                        pcieResults["expectedSpeed"] = int(speedMatch.group(1))
                        self.logger.info(f"Maximum Link Speed: gen {pcieResults['expectedSpeed']}")
                except (ValueError, AttributeError):
                    self.logger.warning(f"Could not parse max speed: {maxSpeedResult['output']}")
            else:
                self.logger.error(f"Failed to get max link speed: {maxSpeedResult['error']}")
            
            # Get current link width (actual)
            self.logger.info("Getting current PCIe link width...")
            widthResult = self.runDgDiag("-PCIE.UTIL.GetLinkWidth")
            if widthResult["success"]:
                try:
                    widthMatch = re.search(r'Current Link Width\s*:\s*x(\d+)', widthResult["output"])
                    if widthMatch:
                        pcieResults["actualWidth"] = int(widthMatch.group(1))
                        self.logger.info(f"Current Link Width: x{pcieResults['actualWidth']}")
                except (ValueError, AttributeError):
                    self.logger.warning(f"Could not parse current width: {widthResult['output']}")
            else:
                self.logger.error(f"Failed to get current link width: {widthResult['error']}")
            
            # Get maximum link width (expected)
            self.logger.info("Getting maximum PCIe link width...")
            maxWidthResult = self.runDgDiag("-PCIE.UTIL.GetMaxLinkWidth")
            if maxWidthResult["success"]:
                try:
                    widthMatch = re.search(r'Max Link Width\s*:\s*x(\d+)', maxWidthResult["output"])
                    if widthMatch:
                        pcieResults["expectedWidth"] = int(widthMatch.group(1))
                        self.logger.info(f"Maximum Link Width: x{pcieResults['expectedWidth']}")
                except (ValueError, AttributeError):
                    self.logger.warning(f"Could not parse max width: {maxWidthResult['output']}")
            else:
                self.logger.error(f"Failed to get max link width: {maxWidthResult['error']}")
            
            # Compare actual vs expected (current vs maximum)
            self.logger.info("Comparing PCIe values:")
            
            # Check if current speed matches maximum capability
            if (pcieResults["actualSpeed"] is not None and 
                pcieResults["expectedSpeed"] is not None):
                pcieResults["speedMatch"] = pcieResults["actualSpeed"] == pcieResults["expectedSpeed"]
                if pcieResults["speedMatch"]:
                    self.logger.info(f"  ✓ Speed: Current gen {pcieResults['actualSpeed']} matches maximum gen {pcieResults['expectedSpeed']} - PASS")
                else:
                    self.logger.warning(f"  ! Speed: Current gen {pcieResults['actualSpeed']} does not match maximum gen {pcieResults['expectedSpeed']} ")
            else:
                self.logger.error("  ✗ Speed: Could not determine speed values - FAIL")
                overallStatus = "FAIL"
            
            # Check if current width matches maximum capability
            if (pcieResults["actualWidth"] is not None and 
                pcieResults["expectedWidth"] is not None):
                pcieResults["widthMatch"] = pcieResults["actualWidth"] == pcieResults["expectedWidth"]
                if pcieResults["widthMatch"]:
                    self.logger.info(f"  ✓ Width: Current x{pcieResults['actualWidth']} matches maximum x{pcieResults['expectedWidth']} - PASS")
                else:
                    self.logger.warning(f"  ! Width: Current x{pcieResults['actualWidth']} does not match maximum x{pcieResults['expectedWidth']}")
            else:
                self.logger.error("  ✗ Width: Could not determine width values - FAIL")
                overallStatus = "FAIL"
            
            # Overall PCIe status - PASS if we got all values
            if (pcieResults["actualSpeed"] is not None and 
                pcieResults["expectedSpeed"] is not None and
                pcieResults["actualWidth"] is not None and 
                pcieResults["expectedWidth"] is not None):
                overallStatus = "PASS"
            else:
                overallStatus = "FAIL"
            
        except Exception as e:
            self.logger.error(f"PCIe test failed: {str(e)}")
            overallStatus = "FAIL"
        
        self.testResults["pcieTestStatus"] = overallStatus
        self.testResults["pcieResults"] = pcieResults
        self.logger.info(f"PCIe Test Status: {overallStatus}")
        return overallStatus
    
    def getMonitoringSummary(self):
        """Get summary of monitoring data"""
        summary = {}
        
        # CPU Speed summary
        if self.monitoringData['cpuSpeeds']:
            speeds = [reading['average'] for reading in self.monitoringData['cpuSpeeds']]
            summary['cpuSpeed'] = {
                'averageMhz': round(sum(speeds) / len(speeds), 2),
                'maxMhz': round(max(speeds), 2),
                'minMhz': round(min(speeds), 2)
            }
        
        # Temperature summary - handle multiple temperature sensors
        if self.monitoringData['temperatures']:
            # Collect all temperature sensor names
            allTempSensors = set()
            for reading in self.monitoringData['temperatures']:
                allTempSensors.update(reading['temperatures'].keys())
            
            summary['temperatures'] = {}
            
            # Calculate stats for each temperature sensor
            for sensorName in allTempSensors:
                sensorReadings = []
                for reading in self.monitoringData['temperatures']:
                    if sensorName in reading['temperatures']:
                        sensorReadings.append(reading['temperatures'][sensorName])
                
                if sensorReadings:
                    summary['temperatures'][sensorName] = {
                        'averageC': round(sum(sensorReadings) / len(sensorReadings), 2),
                        'maxC': round(max(sensorReadings), 2),
                        'minC': round(min(sensorReadings), 2)
                    }
        
        return summary
    
    def writeOutputFile(self):
        """Write results to hostname-based text file"""
        try:
            with open(self.outputFile, 'w') as f:
                f.write(f"Host Test Results\n")
                f.write(f"================\n\n")
                f.write(f"Hostname: {self.getMoboName()}\n")
                f.write(f"Test Date: {self.testResults['testDate']}\n")
                f.write(f"CPU Vendor: {self.testResults['cpuVendor']}\n\n")
                
                # Tool Paths
                f.write(f"Tool Paths:\n")
                f.write(f"DGDiagTool: {self.dgDiagPath.absolute()}\n")
                f.write(f"mprime: {self.mprimePath.absolute()}\n\n")
                
                # PCIe Test Results
                f.write(f"PCIe Test Status: {self.testResults['pcieTestStatus']}\n")
                if 'pcieResults' in self.testResults:
                    pcieData = self.testResults['pcieResults']
                    f.write(f"PCIe Current Speed: gen {pcieData.get('actualSpeed', 'N/A')}\n")
                    f.write(f"PCIe Maximum Speed: gen {pcieData.get('expectedSpeed', 'N/A')}\n")
                    f.write(f"PCIe Current Width: x{pcieData.get('actualWidth', 'N/A')}\n")
                    f.write(f"PCIe Maximum Width: x{pcieData.get('expectedWidth', 'N/A')}\n")
                f.write("\n")
                
                # CPU Benchmark Results
                f.write(f"CPU Benchmark Completed: {self.testResults['cpuBenchmarkCompleted']}\n")
                f.write(f"Benchmark Duration: {self.benchmarkDurationMinutes} minutes\n\n")
                
                # Monitoring Summary
                if 'monitoringSummary' in self.testResults and self.testResults['monitoringSummary']:
                    summary = self.testResults['monitoringSummary']
                    f.write("CPU Performance During Benchmark:\n")
                    f.write("-" * 40 + "\n")
                    
                    # CPU Speed data
                    if 'cpuSpeed' in summary:
                        speedData = summary['cpuSpeed']
                        f.write(f"CPU Speed Average: {speedData['averageMhz']} MHz\n")
                        f.write(f"CPU Speed Maximum: {speedData['maxMhz']} MHz\n")
                        f.write(f"CPU Speed Minimum: {speedData['minMhz']} MHz\n\n")
                    
                    # Temperature data for all sensors
                    if 'temperatures' in summary:
                        f.write("Temperature Readings:\n")
                        for sensorName, tempData in summary['temperatures'].items():
                            f.write(f"{sensorName}:\n")
                            f.write(f"  Average: {tempData['averageC']}°C\n")
                            f.write(f"  Maximum: {tempData['maxC']}°C\n")
                            f.write(f"  Minimum: {tempData['minC']}°C\n")
                else:
                    f.write("No monitoring data available\n")
            
            self.logger.info(f"Results written to: {self.outputFile}")
            
        except Exception as e:
            self.logger.error(f"Failed to write output file: {e}")
    
    def runAllTests(self):
        """Run all tests"""
        self.logger.info(f"Starting tests on {self.getMoboName()}")
        self.logger.info(f"CPU Vendor: {self.cpuVendor}")
        self.logger.info(f"CPU benchmark duration: {self.benchmarkDurationMinutes} minutes")
        
        # Test temperature monitoring before starting
        self.logger.info("Testing temperature monitoring...")
        testTemps = self.getAllCpuTemperatures()
        if testTemps:
            self.logger.info("Available temperature sensors:")
            for sensor, temp in testTemps.items():
                self.logger.info(f"  {sensor}: {temp}°C")
        else:
            self.logger.warning("No temperature sensors detected")
        
        # Run PCIe test
        pcieStatus = self.testPcieSlots()
        
        # Run CPU benchmark
        benchmarkSuccess = self.runCpuBenchmark()
        self.testResults["cpuBenchmarkCompleted"] = benchmarkSuccess
        
        # Get monitoring summary
        self.testResults["monitoringSummary"] = self.getMonitoringSummary()
        
        # Write output file
        self.writeOutputFile()
        
        self.logger.info("Testing completed")
        self.logger.info(f"PCIe Test: {pcieStatus}")
        self.logger.info(f"CPU Benchmark: {'COMPLETED' if benchmarkSuccess else 'FAILED'}")
        self.logger.info(f"Results saved to: {self.outputFile}")
        
        return True

def main():
    parser = argparse.ArgumentParser(description="Host Testing Script")
    parser.add_argument("--log-path", default="./host_tests", help="Path for output files")
    
    args = parser.parse_args()
    
    try:
        tester = HostTester(logPath=args.log_path)
        success = tester.runAllTests()
        sys.exit(0 if success else 1)
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        sys.exit(2)
    except Exception as e:
        print(f"Unexpected error: {str(e)}")
        sys.exit(3)

if __name__ == "__main__":
    main()