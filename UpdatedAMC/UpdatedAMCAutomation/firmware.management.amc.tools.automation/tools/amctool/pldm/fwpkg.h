#ifndef PLDM_FWPKG_H
#define PLDM_FWPKG_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <unistd.h>
#include "../libpldm/base.h"
#include "../libpldm/firmware_update.h"

#define UUID_SIZE 16
#define RELEASE_DATE_TIME_SIZE 13
#define PLDM_FWU_MAX_RECORD_DESCRIPTORS 4
#define PLDM_FWU_MAX_RECORDS 4
#define PLDM_FWU_MAX_IMAGES 4
#define PLDM_DESCRIPTOR_SIZE_BYTES 6
#define FILE_PATH_MAX (4096)

#define offset_of(type, element) ((size_t)&(((type *)0)->element))

typedef struct __attribute__((packed)) {
    uint8_t uuid[UUID_SIZE]; 
    uint8_t revision; 
    uint16_t size; 
    uint8_t release_date_time[RELEASE_DATE_TIME_SIZE]; 
    uint16_t component_bitmap_length;
    uint8_t pkg_version_string_type;
    uint8_t pkg_version_string_length;
    uint8_t pkg_version[PLDM_FWU_COMP_VER_STR_SIZE_MAX];
} firmware_package_hdr_t; 

typedef struct  __attribute__((packed)) {
    uint16_t record_length;
    uint8_t descriptor_count;
    bitfield32_t device_update_option_flags;
    uint8_t component_image_set_ver_str_type;
    uint8_t component_image_set_ver_str_len;
    uint16_t firmware_device_package_data_length;
    uint8_t applicable_components;
    uint8_t component_image_set_ver_str[PLDM_FWU_COMP_VER_STR_SIZE_MAX];
    struct {
        uint16_t descriptor_type;
        uint16_t descriptor_length;
        uint8_t descriptor_data[PLDM_DESCRIPTOR_SIZE_BYTES];
    } record_descriptors[PLDM_FWU_MAX_RECORD_DESCRIPTORS];
}firmware_device_record_t;

typedef struct  __attribute__((packed)) {
    uint8_t device_record_count;
    firmware_device_record_t device_records[PLDM_FWU_MAX_RECORDS]; 
} firmware_device_identification_t;

typedef struct  __attribute__((packed)) {
    uint16_t component_classification;
    uint16_t component_identifier;
    uint32_t component_comparison_stamp;
    bitfield16_t component_options;
    bitfield16_t requested_component_activation_method;
    uint32_t component_location_offset;
    uint32_t component_size;
    uint8_t component_version_string_type;
    uint8_t component_version_string_length;
    uint8_t component_version_string[PLDM_FWU_COMP_VER_STR_SIZE_MAX];
} component_image_info_t;

typedef struct  __attribute__((packed)) {
    uint8_t component_image_count;
    component_image_info_t component_images[PLDM_FWU_MAX_IMAGES];
} component_images_info_t;

typedef struct  __attribute__((packed)) {
    firmware_package_hdr_t hdr;
    firmware_device_identification_t device_identification;
    component_images_info_t component_images_info;
    uint32_t package_info_size; // Size of the firmware package information
    uint32_t checksum; // Optional checksum for integrity
    FILE *fp;
    char file_path[FILE_PATH_MAX]; // Path to the firmware package file
} firmware_package_t;

int fwpkg_parse_package_info(const char *pkg_file_path, firmware_package_t *pkg);
void pkg_info_print(firmware_package_t *pkg);
void fw_image_data_print(const char *pkg_file_path, firmware_package_t *pkg, size_t length);

#endif /* PLDM_FWPKG_H */