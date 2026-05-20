#!/bin/bash

# SPDX-FileCopyrightText: (C) 2025 Intel Corporation
# SPDX-License-Identifier: LicenseRef-Intel

#set -x

# Only pass proxy environment variables to /etc/environment
echo "HTTP_PROXY=${HTTP_PROXY}" | sudo tee /etc/environment >/dev/null
echo "HTTPS_PROXY=${HTTPS_PROXY}" | sudo tee -a /etc/environment >/dev/null
echo "NO_PROXY=${NO_PROXY}" | sudo tee -a /etc/environment >/dev/null

if [ -n "$HTTP_PROXY" ]; then
    echo "Acquire::http::Proxy \"$HTTP_PROXY\";" | sudo tee /etc/apt/apt.conf.d/01proxy >/dev/null
fi

if [ -n "$HTTPS_PROXY" ]; then
    echo "Acquire::https::Proxy \"$HTTPS_PROXY\";" | sudo tee -a /etc/apt/apt.conf.d/01proxy >/dev/null
fi

sudo apt-get update && sudo apt-get install -y udev sshpass fzf socat vim git curl bc sshpass bash
sudo wget https://github.com/mikefarah/yq/releases/latest/download/yq_linux_amd64 -O /usr/local/bin/yq
sudo chmod +x /usr/local/bin/yq

ZEPHYR_SDK_INSTALL_DIR=$(find /opt/toolchains/ -type d -name 'zephyr-sdk-*' | head -n 1 | xargs realpath)
echo "export ZEPHYR_SDK_INSTALL_DIR=$ZEPHYR_SDK_INSTALL_DIR" >>~/.bashrc

cp /workspaces/amc_start.sh /workdir/amc_start.sh
chmod +x /workdir/amc_start.sh
