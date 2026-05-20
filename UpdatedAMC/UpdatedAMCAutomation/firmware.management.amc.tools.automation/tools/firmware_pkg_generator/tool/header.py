#!/usr/bin/env python3

#This tool is inherited from freertos source code as in the below link:
#https://github.com/intel-innersource/firmware.management.amc.amc-gfx-ti

import binascii
import logging
import utils
import re

applicable_components_len = 0
total_len = 0
comp_len = []

def calculate_size(header_binary):
    global total_len
    offset_bytes = len(re.findall(r'WW[A-Fa-f0-9]+XX',header_binary))
    tmp_str = re.sub(r'WW[A-Fa-f0-9]+XX','',header_binary)
    total_len = int((len(tmp_str)-4)/2)+6+(4*offset_bytes)
    return header_binary.replace("ZZZZ",utils.hex_le_uvar(hex(total_len).replace("0x",""),2))

def calculate_record_size(record_data):
    rec_len = hex(int(len(record_data)/2)).replace("0x","")
    return record_data.replace("YYYY",utils.hex_le_uvar(rec_len,2))

def calculate_offset(header_binary):
    global total_len
    global comp_len
    pattern = re.compile(r'WW[A-Fa-f0-9]+XX')
    g = pattern.search(header_binary)
    i = 0
    while g != None:
        start_offset = g.start()+2
        end_offset = g.end()-2
        if start_offset > 0xFFFFFFFF and end_offset > 0xFFFFFFFF:
            return None
        removestr = header_binary[start_offset-2:end_offset+2]
        offset = hex(total_len+int(header_binary[start_offset:end_offset],16)).replace("0x","")
        total_len = total_len+int(comp_len[i],16)
        header_binary = header_binary.replace(removestr, utils.hex_le_uvar(offset,4),1)
        g = pattern.search(header_binary)
    return header_binary

def verify_package_header_information(pldm_header):
    global applicable_components_len
    package_header_information = ""
    if pldm_header['PackageHeaderInformation']['PackageHeaderIdentifier'] == 'F018878CCB7D49439800A02F059ACA02':
        package_header_information += pldm_header['PackageHeaderInformation']['PackageHeaderIdentifier']
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Package Header Identifier is invalid (v1.0.0)!\n")
        return None
    if pldm_header['PackageHeaderInformation']['PackageHeaderFormatRevision'] == '01':
        package_header_information += pldm_header['PackageHeaderInformation']['PackageHeaderFormatRevision']
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Package Header Format Revision is invalid (v1.0.0)!\n")
        return None
    # pldm_header['PackageHeaderInformation']['PackageHeaderSize'] == 'ZZZZ'
    package_header_information += "ZZZZ"
    if len(pldm_header['PackageHeaderInformation']['PackageReleaseDateTime']) == 26 and utils.validate_timestamp104(pldm_header['PackageHeaderInformation']['PackageReleaseDateTime']) is not False:
        package_header_information += pldm_header['PackageHeaderInformation']['PackageReleaseDateTime']
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Package Release Date Time is invalid!\n")
        return None
    if int(pldm_header['PackageHeaderInformation']['ComponentBitmapBitLength'],16)%8 == 0 and int(pldm_header['PackageHeaderInformation']['ComponentBitmapBitLength'],16) >= 0x0000 and int(pldm_header['PackageHeaderInformation']['ComponentBitmapBitLength'],16) <= 0xFFFF:
        package_header_information += utils.hex_le_uvar(pldm_header['PackageHeaderInformation']['ComponentBitmapBitLength'],2)
        applicable_components_len = int(pldm_header['PackageHeaderInformation']['ComponentBitmapBitLength'],16)
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Component Bitmap Bit Length is invalid!\n")
        return None
    if int(pldm_header['PackageHeaderInformation']['PackageVersionStringType'],16) >= 0 and int(pldm_header['PackageHeaderInformation']['PackageVersionStringType'],16) <= 5:
        package_header_information += pldm_header['PackageHeaderInformation']['PackageVersionStringType']
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Package Version String Type is invalid!\n")
        return None
    if int(pldm_header['PackageHeaderInformation']['PackageVersionStringLength'],16) >= 0 and int(pldm_header['PackageHeaderInformation']['PackageVersionStringLength'],16) <= 0xFF:
        package_header_information += pldm_header['PackageHeaderInformation']['PackageVersionStringLength']
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Package Version String Length is invalid!\n")
        return None
    if len(pldm_header['PackageHeaderInformation']['PackageVersionString']) == 2*int(pldm_header['PackageHeaderInformation']['PackageVersionStringLength'],16):
        package_header_information += pldm_header['PackageHeaderInformation']['PackageVersionString']
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Package Version String is invalid!\n")
        return None
    return package_header_information

def verify_firmware_device_identification_area(pldm_header):
    global applicable_components_len
    package_header_information = ""
    number_of_records = 0
    if int(pldm_header['FirmwareDeviceIdentificationArea']['DeviceIDRecordCount'],16) > 0 and int(pldm_header['FirmwareDeviceIdentificationArea']['DeviceIDRecordCount'],16) <= 0xFF:
        package_header_information += pldm_header['FirmwareDeviceIdentificationArea']['DeviceIDRecordCount']
        number_of_records = int(pldm_header['FirmwareDeviceIdentificationArea']['DeviceIDRecordCount'],16)
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Device ID Record Count is invalid!\n")
        return None
    for _ in range(number_of_records):
        record_info = ""
        number_of_descriptors = 0
        fw_dev_pkg_data_len = 0
        # pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordLength'] == 'YYYY'
        record_info += "YYYY"
        if int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['DescriptorCount'],16) > 0 and int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['DescriptorCount'],16) <= 0xFF:
            record_info += pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['DescriptorCount']
            number_of_descriptors = int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['DescriptorCount'],16)
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Descriptor Count is invalid!\n")
            return None
        if int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['DeviceUpdateOptionFlags'],16) == 0 or int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['DeviceUpdateOptionFlags'],16) == 1:
            record_info += utils.hex_le_uvar(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['DeviceUpdateOptionFlags'],4)
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Device Update Option Flags is invalid!\n")
            return None
        if int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionStringType'],16) >= 0 and int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionStringType'],16) <= 5:
            record_info += pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionStringType']
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Image Set Version String Type is invalid!\n")
            return None
        if int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionStringLength'],16) > 0 or int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionStringLength'],16) <= 0xFF:
            record_info += pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionStringLength']
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Image Set Version String Length is invalid!\n")
            return None
        if int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['FirmwareDevicePackageDataLength'],16) >= 0 or int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['FirmwareDevicePackageDataLength'],16) <= 0xFFFF:
            record_info += utils.hex_le_uvar(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['FirmwareDevicePackageDataLength'],2)
            fw_dev_pkg_data_len = int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['FirmwareDevicePackageDataLength'],16)
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Firmware Device Package Data Length is invalid!\n")
            return None
        if len(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ApplicableComponents'])*4 == applicable_components_len:
            record_info += utils.hex_le_uvar(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ApplicableComponents'], int(len(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ApplicableComponents'])/2))
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Applicable Components is invalid!\n")
            return None
        if len(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionString']) == 2*int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionStringLength'],16):
            record_info += pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['ComponentImageSetVersionString']
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Image Set Version String is invalid!\n")
            return None
        for d in range(number_of_descriptors):
            descriptor_info = ""
            descriptor_len = [2,4,16,3,4,3,8]
            if int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorType'],16) >= 0 and int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorType'],16) <= 0x0006:
                descriptor_info += utils.hex_le_uvar(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorType'],2)
                # pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorLength']
                tmp = hex(descriptor_len[int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorType'],16)]).replace("0x","")
                descriptor_info += utils.hex_le_uvar(tmp,2)
            else:
                logging.red("PLDM Firmware Package Error: ")
                logging.white("Descriptor Type is invalid!\n")
                return None
            des_str_len = descriptor_len[int(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorType'],16)]
            if len(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorData']) == 2*des_str_len:
                descriptor_info += utils.hex_le_uvar(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['RecordDescriptors'][d]['DescriptorData'], des_str_len)
            else:
                logging.red("PLDM Firmware Package Error: ")
                logging.white("Record Descriptor is invalid!\n")
                return None
            record_info += descriptor_info
        if len(pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['FirmwareDevicePackageData']) == 2*fw_dev_pkg_data_len:
            record_info += pldm_header['FirmwareDeviceIdentificationArea']['FirmwareDeviceIDRecords'][_]['FirmwareDevicePackageData']
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Firmware Device Package Data is invalid!\n")
            return None
        record_info = calculate_record_size(record_info)
        package_header_information += record_info
    return package_header_information

def verify_component_image_information_area(pldm_header):
    package_header_information = ""
    number_of_components = 0
    global comp_len
    if int(pldm_header['ComponentImageInformationArea']['ComponentImageCount'],16) > 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageCount'],16) <= 0xFFFF:
        package_header_information += utils.hex_le_uvar(pldm_header['ComponentImageInformationArea']['ComponentImageCount'], 2)
        number_of_components = int(pldm_header['ComponentImageInformationArea']['ComponentImageCount'],16)
    else:
        logging.red("PLDM Firmware Package Error: ")
        logging.white("Component Count is invalid!\n")
        return None
    for _ in range(number_of_components):
        comp_info = ""
        comp_opt_bit_1 = False
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentClassification'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentClassification'],16) <= 0xFFFF:
            comp_info += utils.hex_le_uvar(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentClassification'], 2)
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Classification is invalid!\n")
            return None
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentIdentifier'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentIdentifier'],16) <= 0xFFFF:
            comp_info += utils.hex_le_uvar(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentIdentifier'], 2)
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Identifier is invalid!\n")
            return None
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentComparisonStamp'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentComparisonStamp'],16) <= 0xFFFFFFFF:
            comp_info += utils.hex_le_uvar(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentComparisonStamp'], 4)
            if pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentComparisonStamp'] == "FFFFFFFF":
                comp_opt_bit_1 = True
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Comparison Stamp is invalid!\n")
            return None
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentOptions'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentOptions'],16) <= 0x0003:
            if comp_opt_bit_1 == True and (int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentOptions'],16)&0x0002) != 0x0002:
                comp_info += utils.hex_le_uvar(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentOptions'], 2)
            else:
                logging.red("PLDM Firmware Package Error: ")
                logging.white("Component Comparison Stamp is invalid!\n")
                return None
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Option is invalid!\n")
            return None
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['RequestedComponentActivationMethod'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['RequestedComponentActivationMethod'],16) <= 0x003F:
            comp_info += utils.hex_le_uvar(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['RequestedComponentActivationMethod'], 2)
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Request Component Activation Method is invalid!\n")
            return None
        # pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentLocationOffset']
        comp_info += pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentLocationOffset']
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentSize'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentSize'],16) <= 0xFFFFFFFF:
            comp_info += utils.hex_le_uvar(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentSize'], 4)
            comp_len.append(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentSize'])
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Size is invalid!\n")
            return None
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionStringType'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionStringType'],16) <= 5:
            comp_info += pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionStringType']
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Version String Type is invalid!\n")
            return None
        if int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionStringLength'],16) >= 0 and int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionStringLength'],16) <= 0xFF:
            comp_info += pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionStringLength']
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Version String Length is invalid!\n")
            return None
        if len(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionString']) == 2*int(pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionStringLength'],16):
            comp_info += pldm_header['ComponentImageInformationArea']['ComponentImageInformation'][_]['ComponentVersionString']
        else:
            logging.red("PLDM Firmware Package Error: ")
            logging.white("Component Version String is invalid!\n")
            return None
        package_header_information += comp_info
    return package_header_information

def verify_package_header_checksum(header_binary):
    return utils.hex_le_uvar(hex(binascii.crc32(binascii.unhexlify(header_binary))&0xFFFFFFFF).replace("0x",""),4)

def verify_config(pldm_header):
    header_binary = ""
    tmp = verify_package_header_information(pldm_header)
    if tmp is None:
        return None
    else:
        header_binary += tmp
    tmp = verify_firmware_device_identification_area(pldm_header)
    if tmp is None:
        return None
    else:
        header_binary += tmp
    tmp = verify_component_image_information_area(pldm_header)
    if tmp is None:
        return None
    else:
        header_binary += tmp
    header_binary = calculate_size(header_binary)
    header_binary = calculate_offset(header_binary)
    if header_binary is None:
        return None
    header_binary += verify_package_header_checksum(header_binary)
    return header_binary
