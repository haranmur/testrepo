#!/bin/bash

set -e

# --- Board selection from YAML ---
YAML_FILE="/workspaces/boards.yaml"

if [ -f "$YAML_FILE" ]; then
    echo "Available boards:"
    mapfile -t BOARD_NAMES < <(yq '.boards[].name' "$YAML_FILE")
    for i in "${!BOARD_NAMES[@]}"; do
        echo "$((i+1))) ${BOARD_NAMES[$i]}"
    done
    read -t 5 -p "Select board by number (timeout 10s, default 1): " BOARD_IDX
    if [ $? -gt 128 ]; then # timeout occurred
        BOARD_IDX=1
        echo -e "\nTimeout. Defaulting to board 1: ${BOARD_NAMES[0]}"
    fi
    BOARD_IDX=${BOARD_IDX:-1}
    if ! [[ "$BOARD_IDX" =~ ^[0-9]+$ ]] || [ "$BOARD_IDX" -lt 1 ] || [ "$BOARD_IDX" -gt "${#BOARD_NAMES[@]}" ]; then
        echo "Invalid selection. Exiting."
        exit 1
    fi
    BOARD_NAME="${BOARD_NAMES[$((BOARD_IDX-1))]}"
    # Extract all relevant fields from YAML for the selected board
    SUT_IP=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .SUT_IP" "$YAML_FILE"); export SUT_IP
    SUT_PORT=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .SUT_PORT" "$YAML_FILE"); export SUT_PORT
    ENV_SERIAL_PORT="$SUT_IP:$SUT_PORT"; export ENV_SERIAL_PORT
    SUT_PDU_IP=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .SUT_PDU_IP" "$YAML_FILE"); export SUT_PDU_IP
    SUT_PDU_OUTLET=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .SUT_PDU_OUTLET" "$YAML_FILE"); export SUT_PDU_OUTLET
    PDU_USER=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .PDU_USER" "$YAML_FILE"); export PDU_USER
    PDU_PASS=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .PDU_PASS" "$YAML_FILE"); export PDU_PASS
    GDB_SERVER_IP=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .GDB_SERVER_IP" "$YAML_FILE"); export GDB_SERVER_IP
    GDB_SERVER_PORT=$(yq ".boards[] | select(.name == \"$BOARD_NAME\") | .GDB_SERVER_PORT" "$YAML_FILE"); export GDB_SERVER_PORT
    export BOARD_NAME
    echo "Selected board: $BOARD_NAME ($SUT_IP:$SUT_PORT)"
else
    echo "YAML file $YAML_FILE not found. Skipping board selection."
fi

if [ -f "/workspaces/config.env" ]; then
    source "/workspaces/config.env"
else
    echo "Error: config.env file not found!"
    exit 1
fi

select_binary_file() {
    local ALL_BINARIES=( "$@" )
    local PROJECTS_BINARIES=( $(find /workdir/PROJECTS/ -type f -name 'uart*_amc.bin' ! -name '*header*') )
    local TIMEOUT=3
    local choice
    local BINARY_FILE

    if [ ${#ALL_BINARIES[@]} -eq 1 ]; then
        echo "Found single binary: ${ALL_BINARIES[0]}" >&2
        BINARY_FILE="${ALL_BINARIES[0]}"
        sleep 2
        echo "$BINARY_FILE"
        return
    fi

    if [ ${#PROJECTS_BINARIES[@]} -ge 1 ]; then
        echo "Select a Binary File (enter the number, or wait $TIMEOUT seconds to auto-select PROJECTS binary):" >&2
    else
        echo "Select a Binary File (enter the number, required):" >&2
    fi
    i=1
    for bin in "${ALL_BINARIES[@]}"; do
        echo "$i) $bin" >&2
        i=$((i+1))
    done
    sleep 0.2
    if [ ${#PROJECTS_BINARIES[@]} -ge 1 ]; then
        printf "Enter choice [1-%d] (timeout: %d): " ${#ALL_BINARIES[@]} $TIMEOUT >&2
        read -t $TIMEOUT choice
        echo >&2
        if [[ $? -eq 0 && "$choice" =~ ^[0-9]+$ && $choice -ge 1 && $choice -le ${#ALL_BINARIES[@]} ]]; then
            BINARY_FILE="${ALL_BINARIES[$((choice-1))]}"
            echo "Selected: $BINARY_FILE" >&2
        else
            BINARY_FILE="${PROJECTS_BINARIES[0]}"
            echo -e "\nTimeout or invalid input. Auto-selected PROJECTS binary: $BINARY_FILE" >&2
        fi
    else
        printf "Enter choice [1-%d]: " ${#ALL_BINARIES[@]} >&2
        read choice
        echo >&2
        if [[ "$choice" =~ ^[0-9]+$ && $choice -ge 1 && $choice -le ${#ALL_BINARIES[@]} ]]; then
            BINARY_FILE="${ALL_BINARIES[$((choice-1))]}"
            echo "Selected: $BINARY_FILE" >&2
        else
            echo "No valid selection. Exiting." >&2
            exit 1
        fi
    fi
    echo "$BINARY_FILE"
}

PROJECTS_BINARIES=( $(find /workdir/PROJECTS/ -type f -name 'uart*_amc.bin' ! -name '*header*') )
ALL_BINARIES=( $(find /workdir/ -type f -name 'uart*_amc.bin' ! -name '*header*') )

if [ ${#ALL_BINARIES[@]} -eq 0 ]; then
    echo "No binary files found. Exiting."
    exit 1
fi

SELECTED_FILE=$(select_binary_file "${ALL_BINARIES[@]}")
export ENV_BIN_FILE="$SELECTED_FILE"
echo "ENV_SELECTED_FILE set to: $ENV_SELECTED_FILE"

set -x

export ENV_SERIAL_PORT=$ENV_SERIAL_PORT
export ENV_RARITAN_PDU_IP=$SUT_PDU_IP
export ENV_RARITAN_PDU_USERNAME=$PDU_USER
export ENV_RARITAN_PDU_PASSWORD=$PDU_PASS
export ENV_RARITAN_PDU_OUTLETINDEX=$SUT_PDU_OUTLET
echo "ENV_SERIAL_PORT = $ENV_SERIAL_PORT"
cd /workdir/
REPO_URL="https://github.com/intel-innersource/firmware.management.amc.tools.automation.git"
REPO_DIR="val_automation"
set +x
# Clone the repository if not already present
if [ ! -d "$REPO_DIR" ]; then
    echo "Cloning $REPO_URL ..."
    git clone "$REPO_URL" "$REPO_DIR"
    cd "$REPO_DIR"
    git checkout pdu_reboot_included
    cd ..
else
    echo "Repository already cloned: $REPO_DIR"
#    git -C "$REPO_DIR" pull
fi

pwd
cd "$REPO_DIR/automation"

# Install Python requirements
if [ -f "requirements.txt" ]; then
    echo "Installing Python requirements..."
    pip install -r requirements.txt
else
    echo "requirements/requirements.txt not found!"
fi

# Run the main Python script
if [ -f "src/main.py" ]; then
    echo "Running src/main.py ..."
    python src/main.py
else
    echo "src/main.py not found!"
    exit 1
fi
