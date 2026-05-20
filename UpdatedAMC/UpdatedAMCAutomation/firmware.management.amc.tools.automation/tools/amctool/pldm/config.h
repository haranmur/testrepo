#ifndef PLDM_CONFIG_H
#define PLDM_CONFIG_H
#include <stdint.h>
#include <linux/limits.h>

typedef struct
{
    char device[PATH_MAX];    // Device name
    uint8_t slave_addr; // Slave address in hex format
    char image_path[PATH_MAX]; // Path to the firmware image
} amc_config_t;

int parse_amc_config(const char *filepath, amc_config_t *amc_config);

#endif /* PLDM_CONFIG_H */