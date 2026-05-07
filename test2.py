#!/usr/bin/env python3
"""
Automated Host Testing Script
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
    def __init__(self, logPath="./screeningResults"):
        self.logPath = Path(logPath)
        self.monitoringActive = False
        self.monitoringData = {
            'cpuSpeeds': [],
            'temperatures': [],
            'timestamps': []
        }
        
        # Fixed benchmark duration
        self.benchmarkDurationMinutes = 3
        
        # Temperature thresholds
        self.TEMP_WARNING_THRESHOLD = 85.0  # °C
        self.TEMP_CRITICAL_THRESHOLD = 90.0  # °C
        
        # Define tool paths
        self.dgDiagPath = Path.home() / "DGDiagTool_Internal"
        self.mprimePath = Path.home() / "hostTest" / "p95v3019b20.linux64"
        
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
            "pcieTestStatus": {},
            "cpuBenchmarkCompleted": False,
            "monitoringSummary": {},
            "temperatureStatus": {},
            "cpuSpeedStatus": {}
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
    
    def getCpuSpeedLimits(self):
        """Get CPU min/max speeds using lscpu"""
        try:
            result = subprocess.run(['lscpu'], capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                minSpeed = None
                maxSpeed = None
                
                for line in result.stdout.split('\n'):
                    if 'CPU min MHz' in line:
                        try:
                            minSpeed = float(line.split(':')[1].strip())
                        except (ValueError, IndexError):
                            pass
                    elif 'CPU max MHz' in line:
                        try:
                            maxSpeed = float(line.split(':')[1].strip())
                        except (ValueError, IndexError):
                            pass
                
                return minSpeed, maxSpeed
        except Exception as e:
            self.logger.warning(f"Could not get CPU speed limits from lscpu: {e}")
        
        return None, None
    
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
        """Run CPU benchmark with monitoring for set amount of minutes"""
        self.logger.info(f"Starting CPU benchmark for {self.benchmarkDurationMinutes} minutes")
        
        # Check if mprime exists at specified path
        mprimeExe = self.mprimePath / "mprime"
        if not mprimeExe.exists():
            self.logger.warning(f"mprime not found at: {mprimeExe.absolute()}")
        
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
    
    def testPcieSlots(self):
        """Test PCIe Slots with improved pass/fail logic"""
        self.logger.info("=== PCIe Slots Test ===")
        
        pcieStatus = {
            'overall': 'PASS',
            'speedStatus': 'UNKNOWN',
            'widthStatus': 'UNKNOWN',
            'issues': [],
            'actualSpeed': None,
            'expectedSpeed': None,
            'actualWidth': None,
            'expectedWidth': None
        }
        
        try:
            # Get all PCIe values
            self.logger.info("Getting PCIe link information...")
            speedResult = self.runDgDiag("-PCIE.UTIL.GetLinkSpeed")
            maxSpeedResult = self.runDgDiag("-PCIE.UTIL.GetMaxLinkSpeed")
            widthResult = self.runDgDiag("-PCIE.UTIL.GetLinkWidth")
            maxWidthResult = self.runDgDiag("-PCIE.UTIL.GetMaxLinkWidth")
            
            # Parse current speed
            if speedResult["success"]:
                speedMatch = re.search(r'Current Link Speed\s*:\s*Gen(\d+)', speedResult["output"])
                if speedMatch:
                    pcieStatus["actualSpeed"] = int(speedMatch.group(1))
                    self.logger.info(f"Current PCIe Speed: Gen{pcieStatus['actualSpeed']}")
            
            # Parse maximum speed
            if maxSpeedResult["success"]:
                speedMatch = re.search(r'Max Link Speed\s*:\s*Gen(\d+)', maxSpeedResult["output"])
                if speedMatch:
                    pcieStatus["expectedSpeed"] = int(speedMatch.group(1))
                    self.logger.info(f"Maximum PCIe Speed: Gen{pcieStatus['expectedSpeed']}")
            
            # Parse current width
            if widthResult["success"]:
                widthMatch = re.search(r'Current Link Width\s*:\s*x(\d+)', widthResult["output"])
                if widthMatch:
                    pcieStatus["actualWidth"] = int(widthMatch.group(1))
                    self.logger.info(f"Current PCIe Width: x{pcieStatus['actualWidth']}")
            
            # Parse maximum width
            if maxWidthResult["success"]:
                widthMatch = re.search(r'Max Link Width\s*:\s*x(\d+)', maxWidthResult["output"])
                if widthMatch:
                    pcieStatus["expectedWidth"] = int(widthMatch.group(1))
                    self.logger.info(f"Maximum PCIe Width: x{pcieStatus['expectedWidth']}")
            
            # Evaluate speed results
            if pcieStatus["actualSpeed"] is None or pcieStatus["expectedSpeed"] is None:
                pcieStatus['speedStatus'] = 'FAIL'
                pcieStatus['issues'].append('Could not determine PCIe speed values')
                self.logger.error("Failed to get PCIe speed information")
            elif pcieStatus["actualSpeed"] == pcieStatus["expectedSpeed"]:
                pcieStatus['speedStatus'] = 'PASS'
                self.logger.info(f"✓ PCIe Speed: Current Gen{pcieStatus['actualSpeed']} matches maximum Gen{pcieStatus['expectedSpeed']}")
            else:
                # For motherboard compatibility issues, this might be acceptable
                pcieStatus['speedStatus'] = 'WARNING'
                pcieStatus['issues'].append(f'Speed mismatch: Current Gen{pcieStatus["actualSpeed"]} vs Max Gen{pcieStatus["expectedSpeed"]} (may be motherboard limitation)')
                self.logger.warning(f"! PCIe Speed: Current Gen{pcieStatus['actualSpeed']} vs Max Gen{pcieStatus['expectedSpeed']} - may be motherboard limitation")
            
            # Evaluate width results
            if pcieStatus["actualWidth"] is None or pcieStatus["expectedWidth"] is None:
                pcieStatus['widthStatus'] = 'FAIL'
                pcieStatus['issues'].append('Could not determine PCIe width values')
                self.logger.error("Failed to get PCIe width information")
            elif pcieStatus["actualWidth"] == pcieStatus["expectedWidth"]:
                pcieStatus['widthStatus'] = 'PASS'
                self.logger.info(f"✓ PCIe Width: Current x{pcieStatus['actualWidth']} matches maximum x{pcieStatus['expectedWidth']}")
            else:
                pcieStatus['widthStatus'] = 'WARNING'
                pcieStatus['issues'].append(f'Width mismatch: Current x{pcieStatus["actualWidth"]} vs Max x{pcieStatus["expectedWidth"]} (may be motherboard limitation)')
                self.logger.warning(f"! PCIe Width: Current x{pcieStatus['actualWidth']} vs Max x{pcieStatus['expectedWidth']} - may be motherboard limitation")
            
            # Determine overall status
            if pcieStatus['speedStatus'] == 'FAIL' or pcieStatus['widthStatus'] == 'FAIL':
                pcieStatus['overall'] = 'FAIL'
            elif pcieStatus['speedStatus'] == 'WARNING' or pcieStatus['widthStatus'] == 'WARNING':
                pcieStatus['overall'] = 'WARNING'
            else:
                pcieStatus['overall'] = 'PASS'
                
        except Exception as e:
            pcieStatus['overall'] = 'FAIL'
            pcieStatus['issues'].append(f'PCIe test exception: {str(e)}')
            self.logger.error(f"PCIe test failed with exception: {str(e)}")
        
        self.testResults["pcieTestStatus"] = pcieStatus
        self.logger.info(f"PCIe Test Overall Status: {pcieStatus['overall']}")
        return pcieStatus
    
    def evaluateTemperatures(self, tempSummary):
        """Evaluate temperature readings for pass/fail status"""
        tempStatus = {
            'overall': 'PASS',
            'warnings': [],
            'critical': [],
            'maxTemp': 0,
            'criticalSensors': [],
            'warningSensors': []
        }
        
        if not tempSummary:
            tempStatus['overall'] = 'FAIL'
            tempStatus['critical'].append('No temperature data available')
            return tempStatus
        
        self.logger.info("Evaluating temperature readings...")
        
        for sensorName, tempData in tempSummary.items():
            maxTemp = tempData['maxC']
            tempStatus['maxTemp'] = max(tempStatus['maxTemp'], maxTemp)
            
            if maxTemp >= self.TEMP_CRITICAL_THRESHOLD:
                tempStatus['overall'] = 'FAIL'
                tempStatus['critical'].append(f"{sensorName}: {maxTemp}°C")
                tempStatus['criticalSensors'].append(sensorName)
                self.logger.error(f"✗ CRITICAL: {sensorName} reached {maxTemp}°C (threshold: {self.TEMP_CRITICAL_THRESHOLD}°C)")
            elif maxTemp >= self.TEMP_WARNING_THRESHOLD:
                if tempStatus['overall'] != 'FAIL':
                    tempStatus['overall'] = 'WARNING'
                tempStatus['warnings'].append(f"{sensorName}: {maxTemp}°C")
                tempStatus['warningSensors'].append(sensorName)
                self.logger.warning(f"! WARNING: {sensorName} reached {maxTemp}°C (threshold: {self.TEMP_WARNING_THRESHOLD}°C)")
            else:
                self.logger.info(f"✓ {sensorName}: {maxTemp}°C - OK")
        
        self.logger.info(f"Temperature evaluation: {tempStatus['overall']} (Max: {tempStatus['maxTemp']}°C)")
        return tempStatus
    
    def evaluateCpuSpeed(self, speedSummary):
        """Evaluate CPU speed performance for pass/fail status"""
        speedStatus = {
            'overall': 'PASS',
            'issues': [],
            'expectedMin': None,
            'expectedMax': None,
            'actualAvg': None,
            'actualMax': None,
            'performanceRatio': None
        }
        
        if not speedSummary:
            speedStatus['overall'] = 'FAIL'
            speedStatus['issues'].append('No CPU speed data available')
            return speedStatus
        
        self.logger.info("Evaluating CPU speed performance...")
        
        minSpeed, maxSpeed = self.getCpuSpeedLimits()
        speedStatus['expectedMin'] = minSpeed
        speedStatus['expectedMax'] = maxSpeed
        speedStatus['actualAvg'] = speedSummary['averageMhz']
        speedStatus['actualMax'] = speedSummary['maxMhz']
        
        # Check if CPU reached reasonable performance levels
        if maxSpeed:
            # CPU should reach at least 80% of max speed during stress test
            expectedMinPerformance = maxSpeed * 0.8
            speedStatus['performanceRatio'] = (speedSummary['maxMhz'] / maxSpeed) * 100
            
            if speedSummary['maxMhz'] < expectedMinPerformance:
                speedStatus['overall'] = 'FAIL'
                speedStatus['issues'].append(f"Max speed {speedSummary['maxMhz']:.0f}MHz below expected {expectedMinPerformance:.0f}MHz ({speedStatus['performanceRatio']:.1f}% of rated)")
                self.logger.error(f"✗ CPU Speed: {speedSummary['maxMhz']:.0f}MHz < {expectedMinPerformance:.0f}MHz ({speedStatus['performanceRatio']:.1f}% of {maxSpeed:.0f}MHz)")
            else:
                self.logger.info(f"✓ CPU Speed: {speedSummary['maxMhz']:.0f}MHz ({speedStatus['performanceRatio']:.1f}% of {maxSpeed:.0f}MHz rated)")
        else:
            speedStatus['issues'].append('Could not determine CPU speed limits from lscpu')
            self.logger.warning("Could not determine CPU speed limits for evaluation")
        
        self.logger.info(f"CPU speed evaluation: {speedStatus['overall']}")
        return speedStatus
    
    def getMonitoringSummary(self):
        """Get summary of monitoring data"""
        summary = {}
        
        # CPU Speed summary
        if self.monitoringData['cpuSpeeds']:
            speeds = [reading['average'] for reading in self.monitoringData['cpuSpeeds']]
            maxSpeeds = [reading['maximum'] for reading in self.monitoringData['cpuSpeeds']]
            minSpeeds = [reading['minimum'] for reading in self.monitoringData['cpuSpeeds']]
            
            summary['cpuSpeed'] = {
                'averageMhz': round(sum(speeds) / len(speeds), 2),
                'maxMhz': round(max(maxSpeeds), 2),
                'minMhz': round(min(minSpeeds), 2)
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
    
    def getOverallTestStatus(self):
        """Determine overall test status"""
        statuses = []
        
        # Check PCIe
        pcieStatus = self.testResults.get('pcieTestStatus', {})
        if isinstance(pcieStatus, dict):
            statuses.append(pcieStatus.get('overall', 'FAIL'))
        
        # Check CPU benchmark
        if not self.testResults.get('cpuBenchmarkCompleted', False):
            statuses.append('FAIL')
        
        # Check temperatures
        tempStatus = self.testResults.get('temperatureStatus', {})
        if tempStatus:
            statuses.append(tempStatus.get('overall', 'FAIL'))
        
        # Check CPU speed
        speedStatus = self.testResults.get('cpuSpeedStatus', {})
        if speedStatus:
            statuses.append(speedStatus.get('overall', 'FAIL'))
        
        # Determine overall status
        if 'FAIL' in statuses:
            return 'FAIL'
        elif 'WARNING' in statuses:
            return 'WARNING'
        elif 'PASS' in statuses:
            return 'PASS'
        else:
            return 'UNKNOWN'
    
    def writeOutputFile(self):
        """Write results with clear pass/fail status"""
        try:
            with open(self.outputFile, 'w') as f:
                f.write("HOST TEST RESULTS - SUMMARY\n")
                f.write("=" * 50 + "\n\n")
                
                # Overall Status
                overallStatus = self.getOverallTestStatus()
                f.write(f"OVERALL TEST STATUS: {overallStatus}\n")
                f.write(f"Hostname: {self.getMoboName()}\n")
                f.write(f"Test Date: {self.testResults['testDate']}\n")
                f.write(f"CPU Vendor: {self.testResults['cpuVendor']}\n\n")
                
                # Quick Status Summary
                f.write("QUICK STATUS SUMMARY:\n")
                f.write("-" * 30 + "\n")
                
                # PCIe Status
                pcieStatus = self.testResults.get('pcieTestStatus', {})
                if isinstance(pcieStatus, dict):
                    f.write(f"PCIe Test: {pcieStatus.get('overall', 'UNKNOWN')}\n")
                    if pcieStatus.get('issues'):
                        for issue in pcieStatus['issues']:
                            f.write(f"  - {issue}\n")
                    f.write(f"  Current: Gen{pcieStatus.get('actualSpeed', 'N/A')} x{pcieStatus.get('actualWidth', 'N/A')}\n")
                    f.write(f"  Maximum: Gen{pcieStatus.get('expectedSpeed', 'N/A')} x{pcieStatus.get('expectedWidth', 'N/A')}\n")
                else:
                    f.write(f"PCIe Test: {pcieStatus}\n")
                f.write("\n")
                
                # CPU Benchmark Status
                benchmarkStatus = "PASS" if self.testResults['cpuBenchmarkCompleted'] else "FAIL"
                f.write(f"CPU Benchmark: {benchmarkStatus}\n")
                f.write(f"  Duration: {self.benchmarkDurationMinutes} minutes\n")
                
                # CPU Speed Status
                if 'cpuSpeedStatus' in self.testResults:
                    speedStatus = self.testResults['cpuSpeedStatus']
                    f.write(f"CPU Speed Test: {speedStatus['overall']}\n")
                    if speedStatus.get('actualMax') and speedStatus.get('expectedMax'):
                        f.write(f"  Max Speed: {speedStatus['actualMax']:.0f}MHz")
                        if speedStatus.get('performanceRatio'):
                            f.write(f" ({speedStatus['performanceRatio']:.1f}% of rated {speedStatus['expectedMax']:.0f}MHz)")
                        f.write("\n")
                    if speedStatus.get('issues'):
                        for issue in speedStatus['issues']:
                            f.write(f"  - {issue}\n")
                f.write("\n")
                
                # Temperature Status
                if 'temperatureStatus' in self.testResults:
                    tempStatus = self.testResults['temperatureStatus']
                    f.write(f"Temperature Test: {tempStatus['overall']}\n")
                    f.write(f"  Max Temperature: {tempStatus['maxTemp']}°C\n")
                    f.write(f"  Warning Threshold: {self.TEMP_WARNING_THRESHOLD}°C\n")
                    f.write(f"  Critical Threshold: {self.TEMP_CRITICAL_THRESHOLD}°C\n")
                    if tempStatus.get('critical'):
                        f.write(f"  CRITICAL: {', '.join(tempStatus['critical'])}\n")
                    if tempStatus.get('warnings'):
                        f.write(f"  WARNING: {', '.join(tempStatus['warnings'])}\n")
                f.write("\n")
                
                f.write("=" * 50 + "\n")
                f.write("DETAILED RESULTS:\n\n")
                
                # Tool Paths
                f.write(f"Tool Paths:\n")
                f.write(f"DGDiagTool: {self.dgDiagPath.absolute()}\n")
                f.write(f"mprime: {self.mprimePath.absolute()}\n\n")
                
                # Detailed monitoring data
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
        """Run all tests with pass/fail evaluation"""
        self.logger.info(f"Starting tests on {self.getMoboName()}")
        self.logger.info(f"CPU Vendor: {self.cpuVendor}")
        self.logger.info(f"CPU benchmark duration: {self.benchmarkDurationMinutes} minutes")
        self.logger.info(f"Temperature thresholds: Warning {self.TEMP_WARNING_THRESHOLD}°C, Critical {self.TEMP_CRITICAL_THRESHOLD}°C")
        
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
        
        # Get monitoring summary and evaluate
        monitoringSummary = self.getMonitoringSummary()
        self.testResults["monitoringSummary"] = monitoringSummary
        
        # Evaluate temperature performance
        if 'temperatures' in monitoringSummary:
            tempStatus = self.evaluateTemperatures(monitoringSummary['temperatures'])
            self.testResults["temperatureStatus"] = tempStatus
        
        # Evaluate CPU speed performance
        if 'cpuSpeed' in monitoringSummary:
            speedStatus = self.evaluateCpuSpeed(monitoringSummary['cpuSpeed'])
            self.testResults["cpuSpeedStatus"] = speedStatus
        
        # Write output file
        self.writeOutputFile()
        
        # Log summary
        overallStatus = self.getOverallTestStatus()
        self.logger.info("=" * 50)
        self.logger.info("TEST SUMMARY:")
        self.logger.info(f"OVERALL STATUS: {overallStatus}")
        self.logger.info(f"PCIe Test: {pcieStatus.get('overall', 'UNKNOWN')}")
        self.logger.info(f"CPU Benchmark: {'PASS' if benchmarkSuccess else 'FAIL'}")
        
        if 'temperatureStatus' in self.testResults:
            tempStatus = self.testResults['temperatureStatus']
            self.logger.info(f"Temperature Test: {tempStatus['overall']} (Max: {tempStatus['maxTemp']}°C)")
        
        if 'cpuSpeedStatus' in self.testResults:
            speedStatus = self.testResults['cpuSpeedStatus']
            self.logger.info(f"CPU Speed Test: {speedStatus['overall']}")
        
        self.logger.info(f"Results saved to: {self.outputFile}")
        self.logger.info("=" * 50)
        
        return overallStatus in ['PASS', 'WARNING']

def main():
    parser = argparse.ArgumentParser(description="Host Testing Script")
    parser.add_argument("--log-path", default="./screeningResults", help="Path for output files")
    
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