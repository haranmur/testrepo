#!/bin/bash

# INTEL CONFIDENTIAL
# Copyright 2025 Intel Corporation
#
# This software and the related documents are Intel copyrighted materials,
# and your use of them is governed by the express license under which they
# were provided to you (License). Unless the License provides otherwise,
# you may not use, modify, copy, publish, distribute, disclose or
# transmit this software or the related documents without
# Intel's prior written permission.
#
# This software and the related documents are provided as is,
# with no express or implied warranties, other than those
# that are expressly stated in the License.
set -e

#source zephyr/zephyr-env.sh

RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
CYAN='\033[1;36m'
COLOR_OFF='\033[0m'
parentdir=$PWD
basedir=$(dirname "$0")
PROJECT_DIR=$(basename "$basedir")

# Define global variables
REPO_SOCSEC_URL="https://github.com/AspeedTech-BMC/socsec.git"
REPO_SOCSEC_NAME=$(basename $REPO_SOCSEC_URL .git)
SOC_VERSION="v02.00.07"

IMAGE_SIGN_KEY="$parentdir/$REPO_SOCSEC_NAME/tests/keys/ecdsa.pem"
IMAGE_PUB_KEY="$parentdir/$REPO_SOCSEC_NAME/tests/keys/ecdsa.pub.pem"

LTSIGN_DIR="${parentdir}/${PROJECT_DIR}/csslinuxrelease-6.0.0.7-glibc-2.25"
LTSIGN_SCRIPT="$LTSIGN_DIR/ltsign"
LTSIGN_KEY="${parentdir}/${PROJECT_DIR}/lt_signkey.kp"

# Add GFX_IMAGE variables:
GFX_IMAGE_DIR="${parentdir}/gfxbins/"
PLAIN_GFX_IMAGE_FILE="${parentdir}/gfxbins/GFX_FWUpdate.bin"
PLAIN_GFX_IMAGE_FILE_LT_HEADER="${parentdir}/gfxbins/bin_with_header.bin"
PLAIN_GFX_IMAGE_FILE_PADDED="${parentdir}/gfxbins/bin_with_padding.bin"

SIGNED_GFX_IMAGE_FILE="${parentdir}/gfxbins/GFX_FWUpdate_signed.bin"

ALIGNMENT_FOUR_KB=4096
LTCSS_HEADER_SIZE=204

# Calculate padding size for VR image  
HEADER_PADDING_SIZE=$((ALIGNMENT_FOUR_KB - LTCSS_HEADER_SIZE))

#For Beta release, only sign GFX_FWUpdate.bin
GPU_FILE_LIST=("$PLAIN_GFX_IMAGE_FILE")

# Add VR Config variables:
VR_CONFIG_DIR="${parentdir}/vr_config_bins/"
PLAIN_VR_CONFIG_IMAGE_FILE="${parentdir}/vr_config_bins/vr_config.bin"
PLAIN_VR_CONFIG_FILE_LT_HEADER="${parentdir}/vr_config_bins/bin_with_header.bin"
PLAIN_VR_CONFIG_FILE_PADDED="${parentdir}/vr_config_bins/bin_with_padding.bin"
SIGNED_VR_CONFIG_IMAGE_FILE="${parentdir}/vr_config_bins/vr_config_signed.bin"

# Add GFX data variables:
GFX_DATA_DIR="${parentdir}/gfx_data_bins/"
PLAIN_GFX_DATA_IMAGE_FILE="${parentdir}/gfx_data_bins/GFX_Data.bin"
PLAIN_GFX_DATA_FILE_LT_HEADER="${parentdir}/gfx_data_bins/bin_with_header.bin"
PLAIN_GFX_DATA_FILE_PADDED="${parentdir}/gfx_data_bins/bin_with_padding.bin"
SIGNED_GFX_DATA_IMAGE_FILE="${parentdir}/gfx_data_bins/GFX_Data_signed.bin"

# For Beta release, only sign VR_Config.bin
VR_FILE_LIST=("$PLAIN_VR_CONFIG_IMAGE_FILE")

FIRMWARE_TYPE=${1:-"GFX_IMAGE"}  # Default to GFX if no parameter provided
# Validate parameter
if [ "$FIRMWARE_TYPE" != "GFX_IMAGE" ] && [ "$FIRMWARE_TYPE" != "VR" ] && [ "$FIRMWARE_TYPE" != "GFX_DATA" ]; then
    echo -e "${RED}Error: Invalid firmware type '$FIRMWARE_TYPE'. Use GFX_IMAGE, VR, or GFX_DATA.${COLOR_OFF}"
    echo "Usage: $0 [GFX_IMAGE|VR|GFX_DATA]"
    exit 1
fi

echo -e "${CYAN}=== Signing $FIRMWARE_TYPE Firmware ===${COLOR_OFF}"

if [ "$FIRMWARE_TYPE" == "GFX_IMAGE" ]; then
    WORK_DIR="$GFX_IMAGE_DIR"
    FILE_LIST=("${GPU_FILE_LIST[@]}")
    PADDED_FILE="$PLAIN_GFX_IMAGE_FILE_PADDED"
    HEADER_FILE="$PLAIN_GFX_IMAGE_FILE_LT_HEADER"
    PADDING_SIZE="$GFX_IMAGE_PADDING_SIZE"
elif [ "$FIRMWARE_TYPE" == "GFX_DATA" ]; then
    WORK_DIR="$GFX_DATA_DIR"
    FILE_LIST=("$PLAIN_GFX_DATA_IMAGE_FILE")
    PADDED_FILE="$PLAIN_GFX_DATA_FILE_PADDED"
    HEADER_FILE="$PLAIN_GFX_DATA_FILE_LT_HEADER"
elif [ "$FIRMWARE_TYPE" == "VR" ]; then
    WORK_DIR="$VR_CONFIG_DIR"
    FILE_LIST=("${VR_FILE_LIST[@]}")
    PADDED_FILE="$PLAIN_VR_CONFIG_FILE_PADDED"
    HEADER_FILE="$PLAIN_VR_CONFIG_FILE_LT_HEADER"
fi

### Check if directory exists ###
if [ ! -d "$WORK_DIR" ]; then
    echo -e "${RED}Error: $FIRMWARE_TYPE directory '$WORK_DIR' does not exist.${COLOR_OFF}"
    cleanup
    exit 1
fi

# Define the function to clone repo
clonesocsec() {
	# Check if the repository is already cloned
    if [ ! -d "$parentdir/$REPO_SOCSEC_NAME" ]; then
        echo -ne "${YELLOW}Cloning repository $REPO_SOCSEC_URL...${COLOR_OFF}\n"
        git clone $REPO_SOCSEC_URL "$parentdir/$REPO_SOCSEC_NAME"
        if [ $? -ne 0 ]; then
            echo -ne "${RED}Error: Failed to clone repository.${COLOR_OFF}\n"
            exit 1
        fi

        cd $REPO_SOCSEC_NAME
        if [ $? -ne 0 ]; then
            echo -ne "${RED}Error: Failed to change directory into $REPO_SOCSEC_NAME.${COLOR_OFF}\n"
            exit 1
        fi

        git checkout tags/$SOC_VERSION
        if [ $? -ne 0 ]; then
            echo -ne "${RED}Error: Failed to checkout tag $SOC_VERSION.${COLOR_OFF}\n"
            exit 1
        fi
    else
        echo -ne "${GREEN}Repository '$REPO_SOCSEC_NAME' already exists. Skipping clone.${COLOR_OFF}\n"
    fi

    cd "${parentdir}"
    if [ $? -ne 0 ]; then
        echo -ne "${RED}Error: Failed to change back to parent directory.${COLOR_OFF}\n"
        exit 1
    fi
}


# Function to clean up files and directories
cleanup() {

    # Check if the directory exists and delete it
	if [ -d "$parentdir/$REPO_SOCSEC_NAME" ]; then
	    cd "$parentdir"
		rm -rf "$REPO_SOCSEC_NAME"
    fi
	# Check if the directory exists and delete it
	if [ -f "$LTSIGN_KEY" ]; then
	    rm "$parentdir/${PROJECT_DIR}/lt_signkey.kp"
    fi

    if [ -f "${HEADER_FILE}" ]; then
        rm "${HEADER_FILE}"
    fi
    if [ -f "${PADDED_FILE}" ]; then
        rm "${PADDED_FILE}"
    fi
}
################################################################################

### Clone socsec Repo  ###
clonesocsec
if [ $? -ne 0 ]; then
    echo -ne "${RED}error: the clonesocsec function failed.${color_off}\n"
    cleanup
    exit 1
else
    echo -ne "${GREEN}function 'clonesocsec' completed successfully.${color_off}\n"
fi

# Get and ensure  public key key.pem exist
if [ ! -f "$IMAGE_SIGN_KEY" ]; then
    echo "Error: "$IMAGE_SIGN_KEY" not found."
    cleanup
    exit 1
fi

PYTHONPATH="${parentdir}/${PROJECT_DIR}" python3 -c "
from secure_boot_utils import format_key_pemtokp
format_key_pemtokp('${IMAGE_SIGN_KEY}', '${LTSIGN_KEY}')
from secure_boot_utils import extract_ecdsa_qxqy_from_pem
extract_ecdsa_qxqy_from_pem('${IMAGE_PUB_KEY}')
"
if [ $? -ne 0 ]; then
	echo -e "${RED}Error: Conversion from pem to kp  failed.${COLOR_OFF}\n"
	cleanup
	exit 1
else
	echo -e "${GREEN}Conversion from pem to kp completed successfully.${COLOR_OFF}\n"
fi

### Sign Firmware bins  ###
for PLAIN_IMAGE_FILE in "${FILE_LIST[@]}"; do
    echo "Processing $PLAIN_IMAGE_FILE..."
    
    # Get the actual size of the input file
    input_file_size=$(stat -c%s "${PLAIN_IMAGE_FILE}")
    echo "Input file size: $input_file_size bytes"
    
    # Calculate padding needed for 4K alignment (padding at the end)
    remainder=$((input_file_size % ALIGNMENT_FOUR_KB))
    if [ $remainder -eq 0 ]; then
        padding_needed=0
    else
        padding_needed=$((ALIGNMENT_FOUR_KB - remainder))
    fi
    
    echo "Padding needed for 4K alignment: $padding_needed bytes"
    #padding  for first 4k bytes with ltcss header
    dd if=/dev/zero bs=1 count=${HEADER_PADDING_SIZE} of=prefix.bin

    # Create the 4K aligned file by appending padding at the end
    if [ $padding_needed -gt 0 ]; then
        # Copy original file first, then append zero padding
        cat prefix.bin "${PLAIN_IMAGE_FILE}" > "${PADDED_FILE}"
        dd if=/dev/zero bs=1 count=$padding_needed >> "${PADDED_FILE}" 2>/dev/null
    else
        # File is already 4K aligned, just copy it
        cat prefix.bin "${PLAIN_IMAGE_FILE}" > "${PADDED_FILE}"
    fi

    # Get size of binary file
    size=$(stat -c%s "${PADDED_FILE}")
    echo "Size of ${PADDED_FILE} is $size bytes"

    rm prefix.bin

    # Add LTCSS header size
    lt_size=$((size + LTCSS_HEADER_SIZE))
    echo "LT Size of ${PADDED_FILE} is $lt_size bytes"

    # Generate signed output filename dynamically
    SIGNED_IMAGE_FILE="${PLAIN_IMAGE_FILE%.bin}_signed.bin"

    # Create a temp file for the file with header
    cp "${PADDED_FILE}" "${HEADER_FILE}"

    # Add header size to the temp binary file
    python3 "${parentdir}/${PROJECT_DIR}/ltcss_header_add.py" "${HEADER_FILE}" 0 "${lt_size}"
    if [ $? -ne 0 ]; then
        echo -ne "${RED}Failed to add LTCSS header${COLOR_OFF}\n"
        cleanup
        exit 1
    fi

    # Sign the file with header, output to final signed file
    if [ ! -x "$LTSIGN_SCRIPT" ]; then
        echo -e "${RED}Error: LTSign script '$LTSIGN_SCRIPT' not found or not executable.${COLOR_OFF}\n"
        cleanup
        exit 1
    fi
    "$LTSIGN_SCRIPT" --sign -i "${HEADER_FILE}" -k "${LTSIGN_KEY}" -o "${SIGNED_IMAGE_FILE}"
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: LTSign for Image failed.${COLOR_OFF}\n"
        cleanup
        exit 1
    else
        echo -e "${GREEN}LTSign for Image completed successfully.${COLOR_OFF}\n"
    fi
done
cleanup
