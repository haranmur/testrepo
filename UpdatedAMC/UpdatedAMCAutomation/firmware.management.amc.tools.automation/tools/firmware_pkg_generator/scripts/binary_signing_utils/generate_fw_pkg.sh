#!/bin/sh
set -e

# Restrict script execution to the firmware_pkg_generator folder
EXPECTED_DIR="firmware_pkg_generator"
CURRENT_DIR="$(basename "$(pwd)")"

if [ "$CURRENT_DIR" != "$EXPECTED_DIR" ]; then
    echo "Error: This script must be run from the \"$EXPECTED_DIR\" directory." >&2
    exit 1
fi

# Display help if --help argument is passed
if [ "$1" = "--help" ]; then
    echo "Usage: ./scripts/$(basename "$(dirname "$0")")/$(basename "$0") [OPTION] <firmware_type> <input_file.bin>"
    echo "Options:"
    echo "  --help       Display this help message."
    echo "  --clean      Remove the 'out_fw_pkgs' folder as a cleanup activity."
    echo "  --version    Display the version of the Firmware Package Generator."
    echo "  <firmware_type> Type of firmware (e.g., GFX_IMAGE, GFX_DATA, VR)."
    echo "  <input_file.bin> Path to the input binary file."
    exit 0
fi

# Resolve script paths
SCRIPT_PATH="$(cd "$(dirname "$0")" && pwd)"
BASE_PATH="$(cd "$SCRIPT_PATH/../.." && pwd)"
TOOL_PATH="$BASE_PATH/tool/main.py"
OUT_PATH="$BASE_PATH/out_fw_pkgs"
JSON_PATH="$SCRIPT_PATH"
SEC_SCRIPT_PATH="$BASE_PATH/security_prerequisites"

# Check for firmware type parameter
if [ -z "$1" ]; then
    echo "Error: Missing firmware type parameter." >&2
    echo "Usage: $0 [OPTION] <firmware_type> <input_file.bin>" >&2
    echo "Try '$0 --help' for more information." >&2
    exit 1
fi

FIRMWARE_TYPE="$1"
INPUT_BIN_FILE="$2"

# Validate firmware type
if [ "$FIRMWARE_TYPE" != "GFX_IMAGE" ] && [ "$FIRMWARE_TYPE" != "GFX_DATA" ] && [ "$FIRMWARE_TYPE" != "VR" ]; then
    echo "Error: Invalid firmware type '$FIRMWARE_TYPE'. Use GFX_IMAGE, GFX_DATA, or VR." >&2
    echo "Try '$0 --help' for more information." >&2
    exit 1
fi

# Ensure the input binary file is provided
if [ -z "$INPUT_BIN_FILE" ]; then
    echo "Error: Missing input binary file." >&2
    echo "Usage: $0 [OPTION] <firmware_type> <input_file.bin>" >&2
    echo "Try '$0 --help' for more information." >&2
    exit 1
fi

if [ "$FIRMWARE_TYPE" = "GFX_IMAGE" ]; then
    FIRMWARE_PATH="$BASE_PATH/gfx_image_bins"
    FIRMWARE_FILE="GFX_FWUpdate.bin"
    SIGNED_FILE="GFX_FWUpdate_signed.bin"
    JSON_FILE="pldm_fw_package_gpu_ifwi.json"
    OUTPUT_FILE="GFX_fw_pkg.img"
    EXPECTED_SIZE=2101248 # GFX (adjust as needed)
elif [ "$FIRMWARE_TYPE" = "GFX_DATA" ]; then
    FIRMWARE_PATH="$BASE_PATH/gfx_data_bins"
    FIRMWARE_FILE="GFX_Data.bin"
    SIGNED_FILE="GFX_Data_signed.bin"
    JSON_FILE="pldm_fw_package_gpu_data.json"
    OUTPUT_FILE="GFX_Data_fw_pkg.img"
    EXPECTED_SIZE=1355776 # GFX Data (adjust as needed) 
elif [ "$FIRMWARE_TYPE" = "VR" ]; then
    FIRMWARE_PATH="$BASE_PATH/vr_config_bins"
    FIRMWARE_FILE="vr_config.bin"
    SIGNED_FILE="vr_config_signed.bin"
    JSON_FILE="pldm_fw_package_vr_config.json"
    OUTPUT_FILE="VR_fw_pkg.img"
    EXPECTED_SIZE=4064  # VR ( adjust as needed)
fi

# Handle cleanup if --clean argument is passed
if [ "$1" = "--clean" ]; then
    if [ -d "$OUT_PATH" ]; then
        rm -rf "$OUT_PATH"
        echo "The \"$OUT_PATH\" folder has been successfully removed."
    else
        echo "The \"$OUT_PATH\" folder does not exist."
    fi
    exit 0
fi

# Display version if --version argument is passed
if [ "$1" = "--version" ]; then
    VERSION_FILE="$BASE_PATH/VERSION"
    if [ -f "$VERSION_FILE" ]; then
        VERSION=$(cat "$VERSION_FILE")
        echo "Firmware Package Generator Version: $VERSION"
    else
        echo "Version file not found."
    fi
    exit 0
fi

# Check if the provided binary file exists
if [ ! -f "$INPUT_BIN_FILE" ]; then
    echo "Error: The file \"$INPUT_BIN_FILE\" does not exist." >&2
    exit 1
fi

# Check if the input binary file size is exactly 2101248 bytes
FILE_SIZE=$(stat -c%s "$INPUT_BIN_FILE")

if [ "$FILE_SIZE" -ne "$EXPECTED_SIZE" ]; then
    echo "Error: The file \"$INPUT_BIN_FILE\" size is $FILE_SIZE bytes, but it must be exactly $EXPECTED_SIZE bytes for $FIRMWARE_TYPE firmware." >&2
    exit 1
fi

# Check if the security script exists and is executable
if [ ! -x "$SEC_SCRIPT_PATH/lt_sign_fw.sh" ]; then
    echo "Error: Security script '$SEC_SCRIPT_PATH/lt_sign_fw.sh' not found or not executable." >&2
    exit 1
fi

# Check if the tool script exists and is executable
if [ ! -x "$TOOL_PATH" ]; then
    echo "Error: Tool script '$TOOL_PATH' not found or not executable." >&2
    exit 1
fi

# Check if the output directory exists, if not, create it
[ -d "$OUT_PATH" ] || mkdir -p "$OUT_PATH"

# Remove the old bins directory if it exists
[ -d "$FIRMWARE_PATH" ] && rm -rf "$FIRMWARE_PATH"
# Create a new bins directory
mkdir -p "$FIRMWARE_PATH"

cp -avr "$INPUT_BIN_FILE" "$FIRMWARE_PATH/$FIRMWARE_FILE"

# Add LTCSS header to the FPT header removed image
"$SEC_SCRIPT_PATH/lt_sign_fw.sh" "$FIRMWARE_TYPE"

"$TOOL_PATH" -f "$FIRMWARE_PATH/$SIGNED_FILE" -c "$JSON_PATH/$JSON_FILE" -o "$OUT_PATH/$OUTPUT_FILE"

rm -rf "$FIRMWARE_PATH"

echo "The $FIRMWARE_TYPE firmware package has been successfully generated and is located at: $OUT_PATH/$OUTPUT_FILE"
