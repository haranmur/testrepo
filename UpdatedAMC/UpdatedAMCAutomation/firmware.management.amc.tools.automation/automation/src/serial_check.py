import serial.tools.list_ports

def listPorts():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

print(listPorts())