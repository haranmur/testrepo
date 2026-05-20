#!/bin/sh
set -e

# Restrict script execution to the firmware_pkg_generator folder
EXPECTED_DIR="firmware_pkg_generator"
CURRENT_DIR="$(basename "$(pwd)")"

if [ "$CURRENT_DIR" != "$EXPECTED_DIR" ]; then
    echo "Error: This script must be run from the \"$EXPECTED_DIR\" directory." >&2
    exit 1
fi

# Resolve the base directory of the script
SCRIPT_PATH="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_PATH/../.." && pwd)"
JSON_PATH="$SCRIPT_PATH"

# Display help if --help argument is passed
if [ "$1" = "--help" ]; then
    echo "Usage: ./scripts/$(basename "$(dirname "$0")")/$(basename "$0") [OPTION] <input_file.bin>"
    echo "Options:"
    echo "  --help       Display this help message."
    echo "  --clean      Remove the 'out_fw_pkgs' folder as a cleanup activity."
    echo "  --version    Display the version of the Firmware Package Generator."
    echo "  --signed     Create a signed firmware image."
    echo "  --unsigned   Create an unsigned firmware image."
    echo "  <input_file.bin> Path to the input binary file."
    exit 0
fi

# Handle cleanup if --clean argument is passed
if [ "$1" = "--clean" ]; then
    OUT_PATH="$BASE_DIR/out_fw_pkgs"
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
    VERSION_FILE="$BASE_DIR/VERSION"
    if [ -f "$VERSION_FILE" ]; then
        VERSION=$(cat "$VERSION_FILE")
        echo "Firmware Package Generator Version: $VERSION"
    else
        echo "Version file not found."
    fi
    exit 0
fi

# Update JSON file path based on signed or unsigned option
if [ "$1" = "--signed" ]; then
    EXPECTED_SIZE=520192  # 508KB for signed image
    OUTPUT_IMG="amc_signed_fw_pkg.img"
    JSON_FILE="$JSON_PATH/signed_pldm_fw_pkg_amc.json"
elif [ "$1" = "--unsigned" ]; then
    EXPECTED_SIZE=524288  # 512KB for unsigned image
    OUTPUT_IMG="amc_unsigned_fw_pkg.img"
    JSON_FILE="$JSON_PATH/unsigned_pldm_fw_pkg_amc.json"
else
    echo "Error: Invalid option. Use --signed or --unsigned." >&2
    echo "Usage: $0 [--signed|--unsigned] <input_file.bin>" >&2
    exit 1
fi

# Store the option before shifting arguments
OPTION="$1"

# Shift arguments to get the input binary file
shift
INPUT_BIN_FILE="$1"

# Ensure the input binary file is provided
if [ -z "$INPUT_BIN_FILE" ]; then
    echo "Error: Missing or invalid input binary file." >&2
    echo "Usage: $0 [--signed|--unsigned] <input_file.bin>" >&2
    exit 1
fi

# Check if the provided binary file exists
if [ ! -f "$INPUT_BIN_FILE" ]; then
    echo "Error: The file \"$INPUT_BIN_FILE\" does not exist." >&2
    exit 1
fi

# Check the input binary file size
FILE_SIZE=$(stat -c%s "$INPUT_BIN_FILE")
if [ "$FILE_SIZE" -ne "$EXPECTED_SIZE" ]; then
    if [ "$OPTION" = "--signed" ]; then
        echo "Error: The file \"$INPUT_BIN_FILE\" size is $FILE_SIZE bytes, but it must be exactly $EXPECTED_SIZE bytes for a \"signed image\". Please provide a valid signed file." >&2
    elif [ "$OPTION" = "--unsigned" ]; then
        echo "Error: The file \"$INPUT_BIN_FILE\" size is $FILE_SIZE bytes, but it must be exactly $EXPECTED_SIZE bytes for an \"unsigned image\". Please provide a valid unsigned file." >&2
    fi
    exit 1
fi

TOOL_PATH="$BASE_DIR/tool/main.py"
OUT_PATH="$BASE_DIR/out_fw_pkgs"
SEC_SCRIPT_PATH="$BASE_DIR/security_prerequisites"

# Check if the tool script exists and is executable
if [ ! -x "$TOOL_PATH" ]; then
    echo "Error: Tool script '$TOOL_PATH' not found or not executable." >&2
    exit 1
fi

# Check if the output directory exists, if not, create it
[ -d "$OUT_PATH" ] || mkdir -p "$OUT_PATH"

# Generate the firmware package using the appropriate JSON file
$TOOL_PATH -f "$INPUT_BIN_FILE" -c "$JSON_FILE" -o "$OUT_PATH/$OUTPUT_IMG"

echo "The firmware package has been successfully generated and is located at: $OUT_PATH/$OUTPUT_IMG"
