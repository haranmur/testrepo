#!/bin/bash
# set -x

# --- Board selection from YAML ---
YAML_FILE="/workspaces/boards.yaml"

if [ -f "$YAML_FILE" ]; then
    echo "Available boards:"
    mapfile -t BOARD_NAMES < <(yq '.boards[].name' "$YAML_FILE")
    for i in "${!BOARD_NAMES[@]}"; do
        echo "$((i+1))) ${BOARD_NAMES[$i]}"
    done
    read -t 5 -p "Select board by number (timeout 5s, default 1): " BOARD_IDX
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

# Load environment variables from config.env
if [ -f "/workspaces/config.env" ]; then
    source "/workspaces/config.env"
else
    echo "Error: config.env file not found!"
    exit 1
fi


func_reboot_sut_via_pdu() {
    # Intermediate variables for clarity
    PDU_IP="$SUT_PDU_IP"
    PDU_OUTLET="$SUT_PDU_OUTLET"
    PDU_USER="$PDU_USER"
    PDU_PASS="$PDU_PASS"

    SUT_IP="$SUT_IP"
    SUT_PORT="$SUT_PORT"

    # Reboot the Raritan PDU switch via SSH
    echo "Rebooting SUT by PDU[$PDU_IP outlet:$PDU_OUTLET] ..."

    PDU_CMD="power outlets $PDU_OUTLET cycle /y"
    echo "$PDU_CMD" | sshpass -p "$PDU_PASS" ssh -o StrictHostKeyChecking=no "$PDU_USER@$PDU_IP"

    if [ $? -ne 0 ]; then
        echo "Error: Failed to reboot the Raritan PDU switch."
        exit 1
    fi

    # Wait for the PDU outlet to turn back ON
    echo "Waiting for PDU outlet $PDU_OUTLET to turn ON ..."
    for i in {1..15}; do
        sleep 8
        PDU_CMD="show outlets $PDU_OUTLET"
        OUTLET_STATUS=$(echo "$PDU_CMD" | sshpass -p "$PDU_PASS" ssh -o StrictHostKeyChecking=no "$PDU_USER@$PDU_IP")
        echo "$OUTLET_STATUS"
        if echo "$OUTLET_STATUS" | grep -q "Power state: On"; then
            echo "PDU outlet $PDU_OUTLET is ON."
            break
        fi

    done
    # Wait for the PDU switch to reboot
    echo "Waiting for reboot complete ..."
#    sleep 16 # Adjust the sleep time as needed

}


func_check_prompt_and_reboot() {
    local timeout=10
    local result

    # Use socat to connect, send "reset warm", and wait for "amc:~$"
    # Send ENTER first, check for "amc$"
    # Try to connect and read up to 10 lines to search for the prompt
    result=""


    output=$( (echo ""; sleep 1) | socat - TCP4:"$SUT_IP:$SUT_PORT",crnl,connect-timeout=$timeout 10>/dev/null | head -n 10 )
    if [ -n "$output" ]; then
        # Print each line for debugging (optional)
        # echo "$output"
        while IFS= read -r line; do
            if [[ "$line" =~ amc:~\$[[:space:]] ]]; then
                result="$line"
                break
            fi
        done <<< "$output"
    fi
    ## echo "Result: $result"
    if [[ -n "$(echo "$result" | xargs)" ]]; then

        # Wait for user input with timeout
        read -t 5 -n 1 -p $'\e[31mPress \'y\' to do pdu reboot instead of warm (timeout 5s): \e[0m' user_input
        echo
        if [[ "$user_input" == "y" || "$user_input" == "Y" ]]; then
            return 1
        fi

        echo "AMC prompt present: reset cold command ..."
        # If prompt is present, send "reset warm"
        ##output=$( (echo "reset warm"; ) | socat - TCP4:"$SUT_IP:$SUT_PORT",connect-timeout=3 1>/dev/null )
        output=$( (echo "reset cold"; sleep 1) | socat - TCP4:"$SUT_IP:$SUT_PORT" 2>/dev/null )

        sleep 3

        # Take last character of output
        # echo "Output: $output"
        result="${output: -1}"

    fi

    if [[ -n "$(echo "$result" | xargs)" ]]; then
        echo "amc cold rebooted"
        return 0
    else
        return 1
    fi
}

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

# Wait for user input with timeout
read -t 3 -n 1 -p "Press 'y' to do connect serial only (timeout 3s): " user_input1
echo
if [[ "$user_input1" != "y" && "$user_input1" != "Y" ]]; then
    # Auto-select binary in PROJECTS folder if present
    # Search for binaries in PROJECTS folder
    PROJECTS_BINARIES=( $(find /workdir/PROJECTS/ -type f -name 'uart*_amc.bin' ! -name '*header*') )
    # Search for all binaries as fallback
    ALL_BINARIES=( $(find /workdir/ -type f -name 'uart*_amc.bin' ! -name '*header*') )

    if [ ${#ALL_BINARIES[@]} -eq 0 ]; then
        echo "No binary files found. Exiting."
        exit 1
    fi

    BINARY_FILE=$(select_binary_file "${ALL_BINARIES[@]}")
    #   reboot_sut_via_pdu
    if ! func_check_prompt_and_reboot ; then
        echo "AMC prompt is not present . PDU/SUT reboot command ..."
        func_reboot_sut_via_pdu
    fi
    sync
    sleep 10
    echo "Sending '$BINARY_FILE' to serial on $SUT_IP:$SUT_PORT ..."
    socat -d -d FILE:"$BINARY_FILE" TCP:"$SUT_IP:$SUT_PORT",connect-timeout=15

    if [ $? -eq 0 ]; then
        echo -e "\e[32mFile sent successfully.\e[0m"
    else
        echo -e "\e[31mError: Failed to send the file.\e[0m"
        exit 1
    fi

fi
# Open a raw TCP-based serial terminal
echo "Opening TCP serial terminal on $SUT_IP:$SUT_PORT ..."

echo "-------------------------- UART Open ------------------------------------"
echo "-------------------------------------------------------------------------"

socat - TCP4:"$SUT_IP:$SUT_PORT"

if [ $? -eq 0 ]; then
    echo "Raw TCP-based serial terminal session closed."
else
    echo "Error: Failed to open the raw TCP-based serial terminal."
fi
