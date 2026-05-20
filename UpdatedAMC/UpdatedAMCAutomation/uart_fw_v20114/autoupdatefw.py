#Author: Haran Muruganantham

import serial
import time
import subprocess
import os
import sys
from pathlib import Path
import threading
import queue

class FirmwareFlasher:
    def __init__(self):
        self.arduino_connection = None
        self.uart_process = None
        # Get the directory where this script is located
        self.script_dir = os.path.dirname(os.path.abspath(__file__))
        self.uart_exe_path = os.path.join(self.script_dir, "uart_fw_py.exe")
        self.uart_dir = self.script_dir
        self.output_queue = queue.Queue()
        
        # Verify that we're in the correct directory
        if not os.path.exists(self.uart_exe_path):
            print(f"⚠️ Warning: uart_fw_py.exe not found in script directory: {self.script_dir}")
            print("Please ensure this script is placed in the same folder as uart_fw_py.exe")
        
    def get_available_firmware_files(self):
        """Get list of available firmware files in the uart directory"""
        try:
            firmware_files = []
            for file in os.listdir(self.uart_dir):
                if file.endswith('.bin'):
                    firmware_files.append(file)
            return sorted(firmware_files)
        except Exception as e:
            print(f"Error reading firmware directory: {e}")
            return []
    
    def get_user_inputs(self):
        """Get required inputs from user"""
        print("=== Firmware Flashing Setup ===")
        print(f"Script location: {self.script_dir}")
        print(f"UART executable: {self.uart_exe_path}")
        
        # Get Arduino COM port
        arduino_port = input("Enter Arduino COM port (e.g., COM3): ").strip()
        
        # Get UART COM port
        uart_comport = input("Enter UART COM port for flashing (e.g., COM8): ").strip()
        
        # Show available firmware files
        print("\nAvailable firmware files:")
        firmware_files = self.get_available_firmware_files()
        
        if not firmware_files:
            print("No .bin files found in the current directory!")
            print(f"Looking in: {self.uart_dir}")
            return None
        
        for i, file in enumerate(firmware_files, 1):
            print(f"{i}. {file}")
        
        # Let user select firmware file
        while True:
            try:
                choice = input(f"\nSelect firmware file (1-{len(firmware_files)}) or enter filename directly: ").strip()
                
                # Check if it's a number selection
                if choice.isdigit():
                    choice_num = int(choice)
                    if 1 <= choice_num <= len(firmware_files):
                        selected_file = firmware_files[choice_num - 1]
                        break
                    else:
                        print(f"Please enter a number between 1 and {len(firmware_files)}")
                        continue
                
                # Check if it's a direct filename
                elif choice in firmware_files:
                    selected_file = choice
                    break
                
                # Check if it's a partial filename match
                else:
                    matches = [f for f in firmware_files if choice.lower() in f.lower()]
                    if len(matches) == 1:
                        selected_file = matches[0]
                        print(f"Selected: {selected_file}")
                        break
                    elif len(matches) > 1:
                        print("Multiple matches found:")
                        for match in matches:
                            print(f"  - {match}")
                        continue
                    else:
                        print("File not found. Please try again.")
                        continue
                        
            except ValueError:
                print("Invalid input. Please try again.")
                continue
        
        # Validate inputs
        if not arduino_port or not uart_comport:
            print("Error: COM ports are required!")
            return None
            
        if not os.path.exists(self.uart_exe_path):
            print(f"Error: UART executable not found: {self.uart_exe_path}")
            print("Please ensure this script is in the same folder as uart_fw_py.exe")
            return None
        
        # Full path to selected firmware file
        image_path = os.path.join(self.uart_dir, selected_file)
        
        # Ask about looping
        print(f"\n=== Loop Configuration ===")
        loop_mode = input("Do you want to enable loop mode? (y/n): ").strip().lower()
        
        loop_count = 1
        loop_delay = 0
        
        if loop_mode == 'y':
            while True:
                try:
                    loop_input = input("Enter number of loops (or 'infinite' for continuous): ").strip().lower()
                    if loop_input == 'infinite' or loop_input == 'inf':
                        loop_count = -1  # -1 indicates infinite loop
                        break
                    else:
                        loop_count = int(loop_input)
                        if loop_count > 0:
                            break
                        else:
                            print("Please enter a positive number or 'infinite'")
                except ValueError:
                    print("Please enter a valid number or 'infinite'")
            
            while True:
                try:
                    loop_delay = float(input("Enter delay between loops in seconds (0 for no delay): ").strip())
                    if loop_delay >= 0:
                        break
                    else:
                        print("Please enter a non-negative number")
                except ValueError:
                    print("Please enter a valid number")
        
        print(f"\nConfiguration:")
        print(f"Script directory: {self.script_dir}")
        print(f"Arduino COM port: {arduino_port}")
        print(f"UART COM port: {uart_comport}")
        print(f"Firmware file: {selected_file}")
        if loop_count == -1:
            print(f"Loop mode: Infinite loops")
        elif loop_count > 1:
            print(f"Loop mode: {loop_count} loops")
        else:
            print(f"Loop mode: Single flash")
        if loop_count != 1 and loop_delay > 0:
            print(f"Loop delay: {loop_delay} seconds")
        
        confirm = input("\nProceed with flashing? (y/n): ").strip().lower()
        if confirm != 'y':
            print("Flashing cancelled.")
            return None
            
        return {
            'arduino_port': arduino_port,
            'uart_comport': uart_comport,
            'image_path': image_path,
            'image_name': selected_file,
            'loop_count': loop_count,
            'loop_delay': loop_delay
        }
    
    def connect_arduino(self, port):
        """Establish serial connection to Arduino"""
        try:
            self.arduino_connection = serial.Serial(port, 9600, timeout=5)
            time.sleep(2)  # Allow Arduino to reset
            print(f"✓ Connected to Arduino on {port}")
            return True
        except serial.SerialException as e:
            print(f"✗ Failed to connect to Arduino: {e}")
            return False
    
    def enter_boot_mode(self):
        """Step 1: Put GPU into boot mode by sending '1' to Arduino"""
        if not self.arduino_connection:
            print("✗ Arduino not connected!")
            return False
        
        try:
            print("Step 1: Putting GPU into boot mode...")
            self.arduino_connection.write(b'1')
            time.sleep(1)
            
            # Read any response
            if self.arduino_connection.in_waiting > 0:
                response = self.arduino_connection.readline().decode().strip()
                print(f"Arduino response: {response}")
            
            print("✓ GPU should now be in boot mode")
            return True
            
        except Exception as e:
            print(f"✗ Error entering boot mode: {e}")
            return False
    
    def read_uart_output(self, process):
        """Thread function to read UART process output"""
        try:
            while process.poll() is None:
                line = process.stdout.readline()
                if line:
                    self.output_queue.put(('stdout', line.strip()))
                time.sleep(0.1)
        except Exception as e:
            self.output_queue.put(('error', str(e)))
    
    def launch_uart_exe(self, uart_comport, image_path):
        """Step 2 & 3: Launch uart_fw_py.exe with specified parameters"""
        try:
            print("Step 2-3: Launching UART firmware utility...")
            
            # Use just the filename since we're running from the same directory
            image_filename = os.path.basename(image_path)
            
            cmd = [
                "uart_fw_py.exe",
                "--platform", "ast1030",
                "--spi", "0",
                "--cs", "0",
                "--comport", uart_comport,
                "--baudrate", "115200",
                "--input_image", image_filename
            ]
            
            print(f"Executing: {' '.join(cmd)}")
            print(f"Working directory: {self.uart_dir}")
            
            # Launch the process from the uart directory (same as script directory)
            self.uart_process = subprocess.Popen(
                cmd,
                cwd=self.uart_dir,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                universal_newlines=True
            )
            
            # Start output reading thread
            output_thread = threading.Thread(target=self.read_uart_output, args=(self.uart_process,))
            output_thread.daemon = True
            output_thread.start()
            
            print("✓ UART utility launched")
            
            # Give it a moment to start up and show initial output
            print("Reading initial UART output...")
            start_time = time.time()
            
            # Read and display initial output for a few seconds
            while time.time() - start_time < 5:  # Read for 5 seconds
                try:
                    msg_type, output = self.output_queue.get(timeout=0.5)
                    if msg_type == 'stdout':
                        print(f"UART: {output}")
                except queue.Empty:
                    continue
            
            print("✓ UART utility initialized")
            return True
            
        except Exception as e:
            print(f"✗ Error launching UART utility: {e}")
            return False
    
    def accept_flash_mode(self):
        """Step 4: Send 'p' command through Arduino to accept flash mode"""
        if not self.arduino_connection:
            print("✗ Arduino not connected!")
            return False
        
        try:
            print("Step 4: Accepting flash mode...")
            self.arduino_connection.write(b'p')
            time.sleep(1)
            
            # Read any response
            if self.arduino_connection.in_waiting > 0:
                response = self.arduino_connection.readline().decode().strip()
                print(f"Arduino response: {response}")
            
            print("✓ Flash mode accepted")
            return True
            
        except Exception as e:
            print(f"✗ Error accepting flash mode: {e}")
            return False
    
    def start_flashing(self):
        """Step 5 & 6: Send Enter to UART exe to start flashing and wait for completion"""
        if not self.uart_process:
            print("✗ UART process not running!")
            return False
        
        try:
            print("Step 5: Starting flash process...")
            
            # Send Enter to start flashing
            print("Sending Enter to start flashing...")
            self.uart_process.stdin.write('\n')
            self.uart_process.stdin.flush()
            
            print("✓ Flash command sent")
            print("Step 6: Monitoring flashing progress...")
            
            # Monitor the process output and wait for completion
            start_time = time.time()
            timeout = 150  # 2.5 minutes timeout
            last_output_time = start_time
            
            while self.uart_process.poll() is None:
                elapsed = time.time() - start_time
                if elapsed > timeout:
                    print("✗ Flashing timeout!")
                    return False
                
                # Check for output from the process
                try:
                    msg_type, output = self.output_queue.get(timeout=1.0)
                    if msg_type == 'stdout':
                        print(f"UART: {output}")
                        last_output_time = time.time()
                        
                        # Look for completion indicators
                        if "100%" in output or "completed" in output.lower() or "finished" in output.lower():
                            print("✓ Flashing progress: 100%")
                        
                except queue.Empty:
                    # Show periodic progress if no output
                    if time.time() - last_output_time > 15:
                        print(f"Flashing in progress... {int(elapsed)}s elapsed")
                        last_output_time = time.time()
                    continue
            
            # Process has finished, check return code
            return_code = self.uart_process.returncode
            
            # Read any remaining output
            try:
                while True:
                    msg_type, output = self.output_queue.get(timeout=0.5)
                    if msg_type == 'stdout':
                        print(f"UART: {output}")
            except queue.Empty:
                pass
            
            if return_code == 0:
                print("✓ Flashing completed successfully!")
                return True
            else:
                print(f"✗ Flashing process ended with return code: {return_code}")
                return False
                
        except Exception as e:
            print(f"✗ Error during flashing: {e}")
            return False
    
    def exit_boot_mode(self):
        """Step 7: Put GPU back to normal mode by sending '0' to Arduino"""
        if not self.arduino_connection:
            print("✗ Arduino not connected!")
            return False
        
        try:
            print("Step 7: Exiting boot mode...")
            self.arduino_connection.write(b'0')
            time.sleep(1)
            
            # Read any response
            if self.arduino_connection.in_waiting > 0:
                response = self.arduino_connection.readline().decode().strip()
                print(f"Arduino response: {response}")
            
            print("✓ GPU returned to normal mode")
            return True
            
        except Exception as e:
            print(f"✗ Error exiting boot mode: {e}")
            return False
    
    def pulse_gpu_boot(self):
        """Step 8: Send pulse command 'p' to Arduino to boot up the GPU"""
        if not self.arduino_connection:
            print("✗ Arduino not connected!")
            return False
        
        try:
            print("Step 8: Sending pulse to boot up GPU...")
            
            # Send pulse command 'p' (same as flash mode accept)
            self.arduino_connection.write(b'p')
            time.sleep(1)
            
            # Read any response
            if self.arduino_connection.in_waiting > 0:
                response = self.arduino_connection.readline().decode().strip()
                print(f"Arduino response: {response}")
            
            print("✓ Boot pulse sent to GPU")
            print("✓ GPU should now be booting up - check serial connection for boot sequence")
            return True
            
        except Exception as e:
            print(f"✗ Error sending boot pulse: {e}")
            return False
    
    def cleanup(self):
        """Clean up connections and processes"""
        if self.uart_process and self.uart_process.poll() is None:
            try:
                self.uart_process.terminate()
                self.uart_process.wait(timeout=5)
            except:
                self.uart_process.kill()
            print("UART process terminated")
        
        if self.arduino_connection:
            self.arduino_connection.close()
            print("Arduino connection closed")
    
    def single_flash_cycle(self, config):
        """Execute a single flash cycle"""
        try:
            # Step 1: Enter boot mode
            if not self.enter_boot_mode():
                return False
            
            print("Waiting 3 seconds for boot mode to stabilize...")
            time.sleep(3)
            
            # Step 2-3: Launch UART utility
            if not self.launch_uart_exe(config['uart_comport'], config['image_path']):
                return False
            
            print("Waiting 2 seconds before accepting flash mode...")
            time.sleep(2)
            
            # Step 4: Accept flash mode
            if not self.accept_flash_mode():
                return False
            
            print("Waiting 2 seconds before starting flash...")
            time.sleep(2)
            
            # Step 5-6: Start flashing and wait for completion
            if not self.start_flashing():
                return False
            
            print("Waiting 2 seconds before exiting boot mode...")
            time.sleep(2)
            
            # Step 7: Exit boot mode
            if not self.exit_boot_mode():
                return False
            
            print("Waiting 2 seconds before sending boot pulse...")
            time.sleep(2)
            
            # Step 8: Send pulse to boot up GPU
            if not self.pulse_gpu_boot():
                return False
            
            return True
            
        except Exception as e:
            print(f"✗ Error in flash cycle: {e}")
            return False
    
    def flash_firmware(self):
        """Main flashing procedure with loop support"""
        print("=== Automated Firmware Flashing ===\n")
        
        # Get user inputs
        config = self.get_user_inputs()
        if not config:
            return False
        
        # Connect to Arduino once
        if not self.connect_arduino(config['arduino_port']):
            return False
        
        try:
            loop_count = config['loop_count']
            loop_delay = config['loop_delay']
            current_loop = 1
            successful_flashes = 0
            failed_flashes = 0
            
            # Main loop
            while True:
                if loop_count == -1:
                    print(f"\n{'='*50}")
                    print(f"🔄 Starting flash cycle #{current_loop} (Infinite mode)")
                    print(f"{'='*50}")
                else:
                    print(f"\n{'='*50}")
                    print(f"🔄 Starting flash cycle {current_loop}/{loop_count}")
                    print(f"{'='*50}")
                
                # Execute single flash cycle
                cycle_success = self.single_flash_cycle(config)
                
                if cycle_success:
                    successful_flashes += 1
                    print(f"\n✅ Flash cycle {current_loop} completed successfully!")
                else:
                    failed_flashes += 1
                    print(f"\n❌ Flash cycle {current_loop} failed!")
                    
                    # Ask user if they want to continue on failure
                    if loop_count == -1 or current_loop < loop_count:
                        continue_choice = input("Do you want to continue with the next cycle? (y/n): ").strip().lower()
                        if continue_choice != 'y':
                            print("Loop terminated by user due to failure.")
                            break
                
                # Check if we've completed all requested loops
                if loop_count != -1 and current_loop >= loop_count:
                    break
                
                current_loop += 1
                
                # Apply delay between loops (except for the last loop)
                if loop_delay > 0 and (loop_count == -1 or current_loop <= loop_count):
                    print(f"\n⏳ Waiting {loop_delay} seconds before next cycle...")
                    for i in range(int(loop_delay), 0, -1):
                        print(f"Next cycle in {i} seconds... (Press Ctrl+C to stop)", end='\r')
                        time.sleep(1)
                    print(" " * 50, end='\r')  # Clear the countdown line
            
            # Final summary
            print(f"\n{'='*60}")
            print(f"🏁 FLASHING SUMMARY")
            print(f"{'='*60}")
            print(f"Total cycles completed: {current_loop}")
            print(f"Successful flashes: {successful_flashes}")
            print(f"Failed flashes: {failed_flashes}")
            print(f"Success rate: {(successful_flashes/(successful_flashes+failed_flashes)*100):.1f}%" if (successful_flashes+failed_flashes) > 0 else "N/A")
            print(f"Firmware: {config['image_name']}")
            
            if successful_flashes > 0:
                print(f"\n🎉 Loop flashing completed!")
                return True
            else:
                print(f"\n❌ All flash cycles failed!")
                return False
            
        except KeyboardInterrupt:
            print(f"\n\n⚠️ Loop interrupted by user (Ctrl+C)")
            print(f"Completed cycles: {current_loop - 1}")
            print(f"Successful: {successful_flashes}, Failed: {failed_flashes}")
            return False
        except Exception as e:
            print(f"\n✗ Unexpected error: {e}")
            return False
        finally:
            self.cleanup()

def main():
    flasher = FirmwareFlasher()
    success = flasher.flash_firmware()
    
    if success:
        print("\nFlashing process completed successfully!")
    else:
        print("\nFlashing process failed or was interrupted!")
    
    input("\nPress Enter to exit...")

if __name__ == "__main__":
    main()