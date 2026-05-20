cd C:\AMC Automation\firmware.management.amc.tools.automation\automation\src

:loop
python sensorparse.py
timeout /t 10
goto loop