=========================
AMC validation Automation 
=========================

This is a amc automation test project.

Usage
=====

To run the application, use the following steps::
1. Copy the bin file in src folder or results (create ur own).
2. Follow any of the way below.
3. After run complete results will be available on results folder.

Using Native host Python
------------------------

1. Install the required packages::

    ```pip install -r ./requirements.txt```

2. Run the application::

    ```python src/main.py```

Using Docker
------------


1. Build the Docker image::

    ```docker build -t amc-validation-img .```

2. Run the Docker container with the named volume::

    ```docker run --rm -v "$(pwd)/results":/results -v ./requirements.txt:/app/requirements/requirements.txt amc-validation-img```


* Source code mounting can be used for development of pythin script

    ```docker run --rm  -it -v "$(pwd)/src":/app/src -v "$(pwd)/results":/results -v my-requirements:/app/requirements amc-validation-img bash```


example:
    docker run --rm  -it -v "$(pwd)/results":/results -v my-requirements:/app/requirements -e ENV_SERIAL_PORT=$sut_host_ip_port -e SUT_PDU_IP='10.49.11.100' -e PDU_USER=$pdu_username -e PDU_PASS=$pdu_password -e SUT_PDU_OUTLET=$pdu_outlet -e ENV_SERIAL_PORT=$sut_host_ip_port -e HTTP_PROXY=$http_proxy -e HTTPS_PROXY=$https_proxy -e NO_PROXY=$no_proxy -e http_proxy=$http_proxy -e https_proxy=$https_proxy amc-validation-img

Environment Variables (Native host and docker)
==============================================
* ENV_SERIAL_PORT=/dev/ttyUSB0 or THM_IP:THM_TCP_RAW_SERIAL_PORT [THM = NUC connectected to SUT]
* SUT_PDU_IP = 192.168.0.xxx  - Raritan PDU IP (ssh based)
* PDU_USER = '' - Raritan PDU User
* PDU_PASS = '' - Raritan PDU Password
* SUT_PDU_OUTLET = XX - Raritan PDU Outlet Index
* BMC_IP = 192.168.0.xxx - SUT BMC IP (ssh based)
* BMC_USER = '' - SUT BMC Username
* BMC_PASSWORD = '' - SUT BMC Password

Docker Environment Variables (all optional)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* ENV ENV_BIN_FOLDER=/app/src/ -- mandatory for native host run
* ENV ENV_BIN_FILE=uart_amc.bin

Contributing
============

If you would like to contribute, please fork the repository and use a feature branch. Pull requests are warmly welcome.
