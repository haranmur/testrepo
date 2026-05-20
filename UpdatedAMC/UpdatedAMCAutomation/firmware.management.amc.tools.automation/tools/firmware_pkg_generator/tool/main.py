#!/usr/bin/env python3

#This tool is inherited from freertos source code as in the below link:
#https://github.com/intel-innersource/firmware.management.amc.amc-gfx-ti

import json
import sys
import os
import logging
import header
import image

def usage():
    logging.yellow("Command Usage:")
    logging.white("\n{0: <20}".format("Create PLDM Package"))
    logging.white(" : "+sys.argv[0]+" -f <firmware_file_lists> -c <header_json_file> -o <output_file_name>")
    logging.white("\n{0: <20}".format("Help"))
    logging.white(" : "+sys.argv[0]+" -h\n")
    sys.exit(1)

def usage_error():
    logging.red("Usage Error!\n")
    usage()

def add_pldm_header_package_to_binary():
    firmware_file = []
    if sys.argv[1] == '-f':
        firmware_file = sys.argv[2].split(",")
    elif sys.argv[3] == '-f':
        firmware_file = sys.argv[4].split(",")
    elif sys.argv[5] == '-f':
        firmware_file = sys.argv[6].split(",")
    else:
        usage_error()

    for _ in firmware_file:
        if not os.path.isfile(_):
            usage_error()

    if sys.argv[1] == '-c' and os.path.isfile(sys.argv[2]):
        config_file = sys.argv[2]
    elif sys.argv[3] == '-c' and os.path.isfile(sys.argv[4]):
        config_file = sys.argv[4]
    elif sys.argv[5] == '-c' and os.path.isfile(sys.argv[6]):
        config_file = sys.argv[6]
    else:
        usage_error()

    if sys.argv[1] == '-o' and sys.argv[2] != "":
        output_file = os.path.join(os.getcwd(), sys.argv[2])
    elif sys.argv[3] == '-o' and sys.argv[4] != "":
        output_file = os.path.join(os.getcwd(), sys.argv[4])
    elif sys.argv[5] == '-o' and sys.argv[6] != "":
        output_file = os.path.join(os.getcwd(), sys.argv[6])
    else:
        usage_error()

    try:
        pldm_header = json.loads(open(config_file, 'r').read())
    except json.decoder.JSONDecodeError:
        logging.red("Invalid json format: ")
        logging.white(config_file+"\n")
        sys.exit(2)

    header_hex_data = header.verify_config(pldm_header)
    if header_hex_data is None:
        logging.red("Header Generation Error: ")
        logging.white("Failed to create PLDM Firmware Package\n")
        sys.exit(3)
    image.create_pldm_image(firmware_file, header_hex_data, output_file)
    logging.green("Success: ")
    logging.white(output_file+" created!\n")

if __name__ == '__main__':
    if len(sys.argv) == 2 and sys.argv[1] == '-h':
        usage()
    elif len(sys.argv) == 7:
        add_pldm_header_package_to_binary()
    else:
        usage_error()
