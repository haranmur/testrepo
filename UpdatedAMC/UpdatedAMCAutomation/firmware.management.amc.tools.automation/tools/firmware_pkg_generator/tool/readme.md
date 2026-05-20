# PLDM Firmware Packaging Tool

This tool used a json file create PLDM Firmware Package `(Refer DMTF DSP0267 v1.0.0)`.

## Overview of json file

The json file is used generate PLDM Package Header. Assumptions for the json file used by the tool are:

1) All json fields are hex characters. Some special characters are used. Refer `Special Character` section.
2) `UUID`, `Variable` and `Timestamp104` fields are already in little endian. `Timestamp104` strings can be generated using the timestamp104 tool.
3) `uint8`, `enum8` is used without modifications as its value is same in little endian format.
4) `uint16`, `uint32`, `bitfield16`, `bitfield32`, `variable bitfield` will be converted to little endian format.
5) All fields except `Special Character` can be modified, but must be of exact length as specified in the `DMTF DSP0267 v1.0.0`.

### Examples:

1) If `unit16` field has a value `4096 (in hex 0x1000)`. In json file this value will be represented as `1000`.
The tool will convert it to `0100` automatically.
2) If `variable` field has a value `32000000` then the tool will not convert this field to little endian.
The value used will be `32000000` (unchanged).

### Special Character

This section tells the list of special characters in json file that will be dynamically assigned values:

1) In `PackageHeaderInformation -> PackageHeaderSize` the value is `ZZZZ`. This means it's a uint16 field and the value will be computed after parsing all other json field values.
2) In `FirmwareDeviceIdentificationArea -> FirmwareDeviceIDRecords -> RecordLength` the value is `YYYY`. This means it's a uint16 field and the value will be computed after each `FirmwareDeviceIDRecords` is parsed.
3) In `ComponentImageInformationArea -> ComponentImageInformation -> ComponentLocationOffset` the value is `WW<offset>XX`. Here the `<offset>` will have offset value from the input firmware file provided starting from 0. The tool will add the header size to that offset accordingly. `WW` and `XX` are used to detect these fields.
4) In `PackageHeaderChecksum -> PackageHeaderChecksum` the value is `IIIIIIII`. This field is `CRC-32` field which will be computed at the end of the parsing and replacing the other special character fields.

<b>Note</b>: In (3) in the list above only <offset> can be modified. Example, `WW0XX` means:

- `offset = 0` in original firmware file
- `offset = 0+<PLDM Header Size>` in the PLDM FW Package
- `WW` and `XX` are used as delimiters only and will be not be present in PLDM FW Package

## Usage

### Help Command
`python3 main.py -h` or `./main.py -h`
### Generating PLDM Firmware Package.
`./main.py -f <original_firmware_files> -o <Generated PLDM Firmware File Name> -c <JSON File>`

Here, the `<>` are user specified value as explained below:

1) `<original_firmware_files>`: Comma seperated list of component images that must be used to create the PLDM FW Package.
2) `<Generated PLDM Firmware File Name>`: The output file in PLDM Firmware Package format.
3) `<JSON File>`: JSON files containing the PLDM Header data.
