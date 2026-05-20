import os

serialPort = 'COM11'

com_port = os.getenv('ENV_SERIAL_PORT') if os.getenv('ENV_SERIAL_PORT') is not None else serialPort
com_port_baudrate = 115200

# TODO : need to add random values
# Command configurations
fru_set_manufacturer_intel = "fru set_manufacturer intel"
fru_set_product_series_melville_sound = "fru set_product_series melville_sound"
fru_set_serial_number = "fru set_serial_number 987654321"
fru_set_part_number = "fru set_part_number XXX-999"
fru_set_card_type = "fru set_card_type 0"
fru_set_tile_info = "fru set_tile_info 1"
fru_set_platform_type = "fru set_platform_type 1"
fru_set_fab_id = "fru set_fab_id 2"
fru_set_product_name = "fru set_product_name intel"
fru_set_hw_rev = "fru set_hw_rev 0"
fru_set_odm = "fru set_odm 0"
fru_set_card_tdp = "fru set_card_tdp -1"
fru_set_uuid = "fru set_uuid ff"
fru_set_crc = "fru set_crc 0"
fru_set_amc_slave_addr = "fru set_amc_slave_addr 0xFF"
fru_set_fru_file_id = "fru set_fru_file_id 256"
fru_set_mfg_date = "fru set_mfg_date 2025-15-31 31:63"
fru_set_rework_tracker = "fru set_rework_tracker ffffffaa"
fru_set_disable_write_fru = "fru set_disable_write_fru 0"

# Expected output strings
manufacturer_check = "Manufacturer             : intel"
product_series_check = "Product Series           : melville_sound"
serial_number_check = "Serial number            : 987654321"
part_number_check = "Part number              : xxx-999"
card_type_check = "Card Type                : 0"
tile_info_check = "Tile Info                : 1"
platform_type_check = "Platform Type            : 1"
fab_id_check = "Fab ID                   : 2"
product_name_check = "Product Name             : intel"
hw_rev_check = "Hardware Revision        : 0"
odm_check = "ODM                      : 0"
card_tdp_check = "Card TDP                 : -1"
uuid_check = "UUID                     : ff"
crc_check = "CRC                      : 0"
amc_slave_addr_check = "AMC Slave Address        : ff"
fru_file_id_check = "FRU File ID              : 0"
mfg_date_check = "Manufacturing date       : 2025-15-05 31:00"
rework_tracker_check = "Rework Tracker           : ffffffaa"
disable_write_fru_check = "Disable Write Fru        : 0"

# Common configurations
size = 5 * 1024  # Buffer size for reading serial output
enter_1 = "\r\n"  # Enter command sequence

# PDU configurations
raritan_pdu_ip = os.getenv('SUT_PDU_IP') if os.getenv('SUT_PDU_IP') is not None else None
raritan_pdu_username = os.getenv('PDU_USER') if os.getenv('PDU_USER') is not None else 'ci_user'
raritan_pdu_password = os.getenv('PDU_PASS') if os.getenv('PDU_PASS') is not None else 'ci_password'
raritan_pdu_outletIndex = os.getenv('SUT_PDU_OUTLET') if os.getenv('SUT_PDU_OUTLET') is not None else '-1'

# BMC configurations
bmc_ip = os.getenv('BMC_IP') if os.getenv('BMC_IP') is not None else None
bmc_user = os.getenv('BMC_USER') if os.getenv('BMC_USER') is not None else 'root'
bmc_password = os.getenv('BMC_PASSWORD') if os.getenv('BMC_PASSWORD') is not None else None
