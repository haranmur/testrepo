#include <stdio.h>
#include "../pldm/pldm.h"

int fwpkg_parse_package_info(const char *pkg_file_path, firmware_package_t *pkg)
{
    pkg->fp = NULL;

	if (pkg_file_path == NULL || pkg == NULL) {
		fprintf(stderr, "Invalid arguments\n");
		return -EINVAL;
	}

	FILE *fp = fopen(pkg_file_path, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open package file: %s\n", pkg_file_path);
		return -ENOENT;
	}

    uint8_t *pkgbuf = (uint8_t *)malloc(sizeof(firmware_package_t));

    if (pkgbuf == NULL) {
        fprintf(stderr, "Memory allocation failed for firmware package\n");
        fclose(fp);
        pkg->fp = NULL;
        return -ENOMEM;
    }

    fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (file_size < sizeof(firmware_package_t)) {
		fprintf(stderr, "Package file is too small\n");
        free(pkgbuf);
		fclose(fp);
        pkg->fp = NULL;
		return -EINVAL;
	}

	size_t read_size = fread(pkgbuf, 1, sizeof(firmware_package_t), fp);
	if (read_size != sizeof(firmware_package_t)) {
		fprintf(stderr, "Failed to read package file\n");
        free(pkgbuf);
        fclose(fp);
        pkg->fp = NULL;
		return -EIO;
	}

    uint8_t *pbuf = pkgbuf;   

    memcpy(&pkg->hdr, pbuf, offset_of(firmware_package_hdr_t, pkg_version));
    memcpy(pkg->hdr.pkg_version, 
           pbuf + offset_of(firmware_package_hdr_t, pkg_version), 
           pkg->hdr.pkg_version_string_length);

    int next_offset = offset_of(firmware_package_hdr_t, pkg_version) + 
            pkg->hdr.pkg_version_string_length;

    pbuf += next_offset;

    pkg->device_identification.device_record_count = *pbuf;
    if ( pkg->device_identification.device_record_count > PLDM_FWU_MAX_RECORDS) {
        fprintf(stderr, "Device record count exceeds maximum limit\n");
        free(pkgbuf);
        fclose(fp);
        pkg->fp = NULL;
        return -EINVAL;
    }
    pbuf++;

    for (int i = 0; i <  pkg->device_identification.device_record_count; i++) {
        firmware_device_record_t *record = &pkg->device_identification.device_records[i];
        memcpy(record, pbuf, offset_of(firmware_device_record_t, component_image_set_ver_str));
        record->device_update_option_flags.value = record->device_update_option_flags.value;
        record->firmware_device_package_data_length = record->firmware_device_package_data_length;
        pbuf += offset_of(firmware_device_record_t, component_image_set_ver_str);

        memcpy(record->component_image_set_ver_str, pbuf,  
               record->component_image_set_ver_str_len);

        pbuf += record->component_image_set_ver_str_len;

        for (int j = 0; j < record->descriptor_count; j++) {
            record->record_descriptors[j].descriptor_type = *(uint16_t *)pbuf;
            record->record_descriptors[j].descriptor_length = *(uint16_t *)(pbuf + 2);
            pbuf += 4; 

            if (record->record_descriptors[j].descriptor_length > PLDM_DESCRIPTOR_SIZE_BYTES) {
                fprintf(stderr, "Descriptor length exceeds maximum limit\n");
                free(pkgbuf);
                fclose(fp);
                pkg->fp = NULL;
                return -EINVAL;
            }
            if (record->record_descriptors[j].descriptor_length > 0) {
                memcpy(record->record_descriptors[j].descriptor_data, pbuf, 
                       record->record_descriptors[j].descriptor_length);
                pbuf += record->record_descriptors[j].descriptor_length;
            } else {
                memset(record->record_descriptors[j].descriptor_data, 0, PLDM_DESCRIPTOR_SIZE_BYTES);
            }
        }
    }

    component_images_info_t *comp_images_info = &pkg->component_images_info;
    comp_images_info->component_image_count = *pbuf;
    if (comp_images_info->component_image_count > PLDM_FWU_MAX_IMAGES) {
        fprintf(stderr, "Component image count exceeds maximum limit\n");
        free(pkgbuf);
        fclose(fp);
        pkg->fp = NULL; 
        return -EINVAL;
    }
    pbuf += 2;
    for (int i = 0; i < comp_images_info->component_image_count; i++) {
        component_image_info_t *comp_image = &comp_images_info->component_images[i];
        memcpy(comp_image, pbuf, offset_of(component_image_info_t, component_version_string));
        pbuf += offset_of(component_image_info_t, component_version_string);

        if (comp_image->component_version_string_length > PLDM_FWU_COMP_VER_STR_SIZE_MAX) {
            fprintf(stderr, "Component version string length exceeds maximum limit\n");
            free(pkgbuf);
            fclose(fp);
            pkg->fp = NULL;
            return -EINVAL;
        }
        memcpy(comp_image->component_version_string, pbuf, comp_image->component_version_string_length);
        pbuf += comp_image->component_version_string_length;
    }

    pkg->checksum = *(uint32_t *)pbuf;
    pbuf += sizeof(uint32_t);

    pkg->package_info_size = pbuf - pkgbuf;

    strncpy(pkg->file_path, pkg_file_path, FILE_PATH_MAX);
    pkg->file_path[FILE_PATH_MAX - 1] = '\0'; // Ensure null termination

    free(pkgbuf);
    pkg->fp = fp;

	return 0;
}

void pkg_info_print(firmware_package_t *pkg)
{
    if (pkg == NULL) {
        fprintf(stderr, "Invalid package info\n");
        return;
    }
    printf("Firmware Package UUID: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", pkg->hdr.uuid[i]);
    }
    printf("\nRevision: %d\n", pkg->hdr.revision);
    printf("Size: %d\n", pkg->hdr.size);
    printf("Release Date Time: ");
    for (int i = 0; i < 13; i++) {
        printf("%02x", pkg->hdr.release_date_time[i]);
    }
    printf("\nComponent Bitmap Length: %d\n", pkg->hdr.component_bitmap_length);
    printf("Package Version String Type: %d\n", pkg->hdr.pkg_version_string_type);
    printf("Package Version String Length: %d\n", pkg->hdr.pkg_version_string_length);
    printf("Package Version: ");
    for (int i = 0; i < pkg->hdr.pkg_version_string_length; i++) {
        printf("%02x", pkg->hdr.pkg_version[i]);
    }
    printf("\n\n");

    firmware_device_identification_t *device_identification = &pkg->device_identification;

    printf("Device Record Count: %d\n", device_identification->device_record_count);
    for (int i = 0; i < device_identification->device_record_count; i++) {
        firmware_device_record_t *record = &device_identification->device_records[i];
        printf("Record %d:\n", i);
        printf("  Record Length: %d\n", record->record_length);
        printf("  Descriptor Count: %d\n", record->descriptor_count);
        printf("  Device Update Option Flags: %08x\n", be32toh(record->device_update_option_flags.value));
        printf("  Component Image Set Version String Type: %d\n", record->component_image_set_ver_str_type);
        printf("  Component Image Set Version String Length: %d\n", record->component_image_set_ver_str_len);
        printf("  Firmware Device Package Data Length: %d\n", be16toh(record->firmware_device_package_data_length));
        printf("  Applicable Components: %d\n", record->applicable_components);
        printf("  Component Image Set Version String: ");
        for (int j = 0; j < record->component_image_set_ver_str_len; j++) {
            printf("%02x", record->component_image_set_ver_str[j]);
        }
        for (int j = 0; j < record->descriptor_count; j++) {
            printf("\n    Descriptor %d:\n", j);
            printf("      Descriptor Type: %d\n", record->record_descriptors[j].descriptor_type);
            printf("      Descriptor Length: %d\n", record->record_descriptors[j].descriptor_length);
            printf("      Descriptor Data: ");
            for (int k = 0; k < record->record_descriptors[j].descriptor_length; k++) {
                printf("%02x ", record->record_descriptors[j].descriptor_data[k]);
            }
        }
        if (record->firmware_device_package_data_length > 0) {
            printf("\n  Firmware Device Package Data: ");
            for (int k = 0; k < record->firmware_device_package_data_length; k++) {
                printf("%02x ", record->component_image_set_ver_str[k]);
            }
        }
        printf("\n");
    }
    printf("Component Image Count: %d\n", pkg->component_images_info.component_image_count);
    for (int i = 0; i < pkg->component_images_info.component_image_count; i++) {
        component_image_info_t *comp_image = &pkg->component_images_info.component_images[i];
        printf("Component Image %d:\n", i);
        printf("  Component Classification: %d\n", comp_image->component_classification);
        printf("  Component Identifier: %d\n", comp_image->component_identifier);
        printf("  Component Comparison Stamp: %d\n", comp_image->component_comparison_stamp);
        printf("  Component Options: %04x\n", comp_image->component_options.value);
        printf("  Requested Component Activation Method: %d\n", comp_image->requested_component_activation_method.value);
        printf("  Location Offset: %d\n", comp_image->component_location_offset);
        printf("  Component Size: %d\n", comp_image->component_size);
        printf("  Component Version String Type: %d\n", comp_image->component_version_string_type);
        printf("  Component Version String Length: %d\n", comp_image->component_version_string_length);
        printf("  Component Version String: ");
        for (int j = 0; j < comp_image->component_version_string_length; j++) {
            printf("%02x", comp_image->component_version_string[j]);
        }
        printf("\n");
    }

    printf("Checksum: %08x\n", pkg->checksum);

    printf("Firmware Package Size: %u bytes\n", pkg->package_info_size);
    printf("Firmware Package File Path: %s\n", pkg->file_path);
    printf("Firmware Package Info Dump Complete\n");

    printf("\n");
}

void fw_image_data_print(const char *pkg_file_path, firmware_package_t *pkg, size_t length)
{
    if (pkg == NULL) {
        fprintf(stderr, "Invalid package info\n");
        return;
    }

    if (pkg_file_path == NULL) {
        fprintf(stderr, "Invalid package file path\n");
        return;
    }
    FILE *fp = fopen(pkg_file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open package file: %s\n", pkg_file_path);
        return;
    }

    uint8_t *image_data = (uint8_t *)malloc(length * sizeof(uint8_t));
    if (image_data == NULL) {   
        fprintf(stderr, "Memory allocation failed for image data\n");
        fclose(fp);
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < length) {
        fprintf(stderr, "File size is smaller than expected image data length\n");
        free(image_data);
        fclose(fp);
        return;
    }

    for (int i = 0; i < pkg->component_images_info.component_image_count; i++) {
        component_image_info_t *comp_image = &pkg->component_images_info.component_images[i];
        printf("  Location Offset: %d\n", comp_image->component_location_offset);
        fseek(fp, comp_image->component_location_offset, SEEK_SET);
        size_t read_size = fread(image_data, 1, length, fp);
        if (read_size != length) {
            fprintf(stderr, "Failed to read expected image data length: %zu\n", read_size);
            free(image_data);
            fclose(fp);
            return;
        }
        printf("  Image Data for Component %d:\n", i);
        for (size_t j = 0; j < length; j++) {
            printf("%02x ", image_data[j]);
            if ((j + 1) % 16 == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }

    fclose(fp);
    free(image_data);
}