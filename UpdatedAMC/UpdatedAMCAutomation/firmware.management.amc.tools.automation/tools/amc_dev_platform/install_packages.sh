#!/bin/bash

# SPDX-FileCopyrightText: (C) 2024 Intel Corporation
# SPDX-License-Identifier: LicenseRef-Intel

# Define your proxy settings
HTTP_PROXY=$http_proxy
HTTPS_PROXY=$https_proxy
NO_PROXY=$noproxy

sudo apt-get update
sudo apt-get install -y ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings

if ! command -v docker &>/dev/null; then
    echo "Docker could not be found, attempting to install Docker..."

    sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
    sudo chmod a+r /etc/apt/keyrings/docker.asc

    # Add the repository to Apt sources:
    echo \
        "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "$VERSION_CODENAME") stable" |
        sudo tee /etc/apt/sources.list.d/docker.list >/dev/null
    sudo apt-get update
    sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
    sudo apt autoremove -y
    sudo groupadd docker
    sudo usermod -aG docker ${USER}

    # Create or modify the Docker systemd service file to include the proxy settings
    sudo mkdir -p /etc/systemd/system/docker.service.d

     # Create a file named http-proxy.conf
  # Create a file named http-proxy.conf
  cat <<EOF | sudo tee /etc/systemd/system/docker.service.d/http-proxy.conf >/dev/null
[Service]
Environment="HTTP_PROXY=$HTTP_PROXY"
Environment="HTTPS_PROXY=$HTTPS_PROXY"
Environment="FTP_PROXY=$FTP_PROXY"
Environment="NO_PROXY=$NO_PROXY"
EOF

  # Reload the systemd daemon to apply the changes
  sudo systemctl daemon-reload

  # Restart the Docker service to use the proxy settings
  sudo systemctl restart docker
  sleep 2
#    newgrp docker
fi

#docker pull hello-world
