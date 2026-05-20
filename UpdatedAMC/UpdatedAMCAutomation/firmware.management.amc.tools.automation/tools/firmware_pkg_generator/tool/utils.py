#!/usr/bin/env python3

#This tool is inherited from freertos source code as in the below link:
#https://github.com/intel-innersource/firmware.management.amc.amc-gfx-ti

import logging
import struct

def valid_day_in_month(day, month):
    if month == 1 or month == 3 or month == 5 or month == 7 or month == 8 or month == 10 or month == 12:
        if day > 31:
            return False
    if month == 4 or month == 6 or month == 9 or month == 11:
        if day > 30:
            return False
    if month == 2:
        if day > 29:
            return False
    return True

def leap_year(year):
    if year%400 == 0:
        return True
    elif year%100 != 0 and year%4 == 0:
        return True
    else:
        return False

def validate_timestamp104(date_type):
    utc_offset = struct.unpack('>h',bytes.fromhex(hex_le_uvar(date_type[0:4],2)))[0]
    if utc_offset > (2**15)-1 or utc_offset < -(2**15):
        logging.red("Timestamp error: ")
        logging.white("UTC offset is invalid!\n")
        return False
    time_ms = int(hex_le_uvar(date_type[4:10],3),16)
    if time_ms > 0xFFFFFF or time_ms < 0:
        logging.red("Timestamp error: ")
        logging.white("Microseconds is invalid!\n")
        return False
    time_s = int(date_type[10:12],16)
    if time_s > 59 or time_s < 0:
        logging.red("Timestamp error: ")
        logging.white("Seconds is invalid!\n")
        return False
    time_m = int(date_type[12:14],16)
    if time_m > 59 or time_m < 0:
        logging.red("Timestamp error: ")
        logging.white("Minute is invalid!\n")
        return False
    time_hr = int(date_type[14:16],16)
    if time_hr > 23 or time_hr < 0:
        logging.red("Timestamp error: ")
        logging.white("Hours is invalid!\n")
        return False
    time_day = int(date_type[16:18],16)
    if time_day > 31 or time_day <= 0:
        logging.red("Timestamp error: ")
        logging.white("Day is invalid!\n")
        return False
    time_mth = int(date_type[18:20],16)
    if time_mth > 12 or time_mth <= 0:
        logging.red("Timestamp error: ")
        logging.white("Day is invalid!\n")
        return False
    if valid_day_in_month(time_day, time_mth) is False:
        logging.red("Timestamp error: ")
        logging.white("Day is invalid!\n")
        return False
    time_yr = int(hex_le_uvar(date_type[20:24],2),16)
    if time_yr > 0xFFFF or time_yr < 0:
        logging.red("Timestamp error: ")
        logging.white("Year is invalid!\n")
        return False
    if time_mth == 2 and leap_year(time_yr) is False:
         if time_day > 28:
            logging.red("Timestamp error: ")
            logging.white("Day is invalid!\n")
            return False
    utc_res = int(date_type[24],16)
    if utc_res > 3 or utc_res < 0:
        logging.red("Timestamp error: ")
        logging.white("UTC Resolution is invalid!\n")
        return False
    tm_res = int(date_type[25],16)
    if tm_res > 13 or tm_res < 0:
        logging.red("Timestamp error: ")
        logging.white("Time Resolution is invalid!\n")
        return False
    return True

def hex_le_uvar(value,no_of_bytes):
    tmp = "{}".format((value).replace("0x","").zfill(2*no_of_bytes))
    chunk_size = 2
    bytes_arr = [ tmp[i:i+chunk_size] for i in range(0,len(tmp),chunk_size)]
    bytes_arr = bytes_arr[::-1]
    bytes = "".join(bytes_arr)
    return bytes
