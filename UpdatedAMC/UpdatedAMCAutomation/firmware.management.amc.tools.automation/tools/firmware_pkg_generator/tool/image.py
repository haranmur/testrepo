#!/usr/bin/env python3

#This tool is inherited from freertos source code as in the below link:
#https://github.com/intel-innersource/firmware.management.amc.amc-gfx-ti

import binascii
import logging

def create_pldm_image(original_image,pldm_package_header,output_file):
    total_bytes = 0
    fout = open(output_file,'wb')
    no_of_header_bytes = len(binascii.unhexlify(pldm_package_header))
    fout.write(binascii.unhexlify(pldm_package_header))
    logging.yellow("Creating Firmware Package:\n")
    logging.purple("    {0: <30}".format("Written PLDM Package Header")+" : ")
    logging.cyan(str(no_of_header_bytes).rjust(8)+"\n")
    total_bytes = no_of_header_bytes
    for _ in range(len(original_image)):
        fin = open(original_image[_],'rb')
        no_of_bytes = 0
        while True:
            byte = fin.read(1)
            if not byte:
                break
            fout.write(byte)
            no_of_bytes += 1
        fin.close()
        total_bytes += no_of_bytes
        logging.purple("    {0: <30}".format("Written Firmware Data")+" : ")
        logging.cyan(str(no_of_bytes).rjust(8)+"\n")
    fout.close()
    logging.purple("    {0: <30}".format("Written Total Bytes")+" : ")
    logging.cyan(str(total_bytes).rjust(8)+"\n")
