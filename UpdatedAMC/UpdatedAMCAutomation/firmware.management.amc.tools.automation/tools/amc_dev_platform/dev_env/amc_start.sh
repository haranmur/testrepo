#!/bin/bash

# SPDX-FileCopyrightText: (C) 2025 Intel Corporation
# SPDX-License-Identifier: LicenseRef-Intel

#set -x

cd /workdir || {
    echo "Failed to change directory to /workdir"
    exit 1
}

mkdir -p PROJECTS
cd PROJECTS || {
    echo "Failed to change directory to PROJECTS"
    exit 1
}
echo "Current directory after cd PROJECTS: $(pwd)"


if [ -d "intel_amc_zephyr" ]; then
    echo "Directory intel_amc_zephyr already exists. Deleting it..."
    rm -rf intel_amc_zephyr zephyr .west modules build
fi

echo "Cloning repository..."
git clone https://github.com/intel-innersource/firmware.management.amc.zephyr.git intel_amc_zephyr

## git checkout remove_hardcodig_folders
#cd ..
if [ $? -ne 0 ]; then
    echo "Failed to clone repository"
    exit 1
fi

# Add a delay to ensure filesystem sync
sleep 5

echo "Current directory after git clone: $(pwd)"
ls
if [ ! -d "intel_amc_zephyr" ]; then
    echo "Directory intel_amc_zephyr does not exist"
    exit 1
fi

pushd intel_amc_zephyr || {
    echo "Failed to change directory to intel_amc_zephyr"
    exit 1
}
popd

# Replace hardcoded "firmware.management.amc.zephyr" with "intel_amc_zephyr" in all files
find intel_amc_zephyr/zephyr_patches -type f -exec sed -i 's/firmware\.management\.amc\.zephyr/intel_amc_zephyr/g' {} +
if [ $? -ne 0 ]; then
    echo "Failed to replace hardcoded strings"
    exit 1
fi

sync

# Print final directory and list contents
echo "To continue, run: cd $(pwd)"

if [ ! -d "zephyr" ]; then
    west init -l --mf west.yml intel_amc_zephyr
    west update
fi
if [ $? -ne 0 ]; then
    echo "Failed to initialize west"
    exit 1
fi
echo -e "\033[40;1;93mCurrent directory after intel_amc repo clone: $(pwd)\033[0m"
echo -e "\033[40;3;93mIntel project folder name: $(pwd)/intel_amc_zephyr \033[0m"
