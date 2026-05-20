#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include <stdbool.h>
#include <error.h>
#include "../libpldm/base.h"

void buf_print(char *header, uint8_t *data, uint8_t len);
void pldm_types_print(uint8_t *types, uint8_t length);
int parse_time_date(uint8_t *ptr, timestamp104_t *timestamp);

#endif // UTILS_H