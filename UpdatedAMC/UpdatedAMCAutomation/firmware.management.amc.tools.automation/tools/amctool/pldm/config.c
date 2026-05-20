#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include "config.h"

int parse_amc_config(const char *filepath, amc_config_t *amc_config)
{
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "%s file open error\n", filepath);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (file_size <= 0)
    {
        fprintf(stderr, "File is empty or error reading file size\n");
        fclose(fp);
        return -1;
    }
    char *buf = malloc(file_size + 1);
    memset(buf, 0, file_size + 1);

    size_t len = fread(buf, 1, file_size, fp);
    if (len != file_size)
    {
        fprintf(stderr, "read error\n");
	fclose(fp);
	free(buf);
        return -1;
    }

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
	fclose(fp);
	free(buf);
        return 1;
    }

    cJSON *device = cJSON_GetObjectItemCaseSensitive(json, "device");
    cJSON *slave_addr = cJSON_GetObjectItemCaseSensitive(json, "slave_addr");
    cJSON *image_path = cJSON_GetObjectItemCaseSensitive(json, "image_path");

    if (!cJSON_IsString(device) || !cJSON_IsString(slave_addr) || !cJSON_IsString(image_path))
    {
        fprintf(stderr, "Invalid JSON format or missing fields\n");
        cJSON_Delete(json);
        fclose(fp);
	free(buf);
        return -1;
    }   
    
    strncpy(amc_config->device, device->valuestring, sizeof(amc_config->device) - 1);
    amc_config->device[strlen(amc_config->device)] = '\0'; // Ensure null termination
    
    strncpy(amc_config->image_path, image_path->valuestring, sizeof(amc_config->image_path) - 1);
    amc_config->image_path[strlen(amc_config->image_path)] = '\0'; // Ensure null termination
    
    amc_config->slave_addr = strtol(slave_addr->valuestring, NULL, 16);
    if (amc_config->slave_addr < 0 || amc_config->slave_addr > 0xFF)
    {
        fprintf(stderr, "Invalid slave address: %s\n", slave_addr->valuestring);
        cJSON_Delete(json);
        fclose(fp);
	free(buf);
        return -1;
    }

    cJSON_Delete(json);
    fclose(fp);
    free(buf);

    return 0;
}
