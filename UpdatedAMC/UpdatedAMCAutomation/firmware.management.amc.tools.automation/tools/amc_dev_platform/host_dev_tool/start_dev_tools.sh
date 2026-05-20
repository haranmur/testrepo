#!/bin/bash

# SPDX-FileCopyrightText: (C) 2025 Intel Corporation
# SPDX-License-Identifier: LicenseRef-Intel

###################################################################
# Script Name : start_dev_tools.sh
#
# Description :
#
# Args :
#
###################################################################

if ! command -v JLinkGDBServer &>/dev/null; then
    echo "JLinkGDBServer could not be found, Installing Jlink tools"
    sudo dpk -i ./JLink_Linux_V812d_x86_64.deb
#    exit 1
fi

if [ -z "$1" ]; then
    read -p "Do you require remote serial? (yes/no): " remote_serial
else
    remote_serial=$1
fi
read -p "Enable JTAG debug server? (yes/no): " is_gdb_server

if [ "$remote_serial" == "yes" ]; then
    echo -e "Starting remote serial... on \e[43;30m/dev/ttyUSB0 listening on port 3345\e[0m"
    # Add commands to start remote serial here
    sudo socat TCP-LISTEN:3345,reuseaddr,fork FILE:/dev/ttyUSB0,raw,echo=0 &
fi

if [ "$is_gdb_server" == "yes" ]; then
echo -e "Starting \e[43;30mGDB server listening on port 5000\e[0m"
JLinkGDBServer -device Cortex-M4 -if jtag -port 5000 -rtos /opt/SEGGER/JLink/GDBServer/RTOSPlugin_Zephyr.so
fi
