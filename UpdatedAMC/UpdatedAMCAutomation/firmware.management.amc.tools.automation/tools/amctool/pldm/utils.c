#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include "../libpldm/base.h"
#include "../libpldm/pldm_types.h"

struct pldm_types {
	uint8_t type;
	char str[30];
} pldm_types[] = {
	{PLDM_BASE, "PLDM Base"}, {PLDM_SMBIOS, "PLDM SMBIOS"}, {PLDM_PLATFORM, "PLDM Platform"},
	{PLDM_BIOS, "PLDM BIOS"}, {PLDM_FRU, "PLDM FRU"},       {PLDM_FWUP, "PLDM FWUP"},
	{PLDM_RDE, "PLDM RDE"},   {PLDM_OEM, "PLDM OEM"},
};

void pldm_types_print(uint8_t *types, uint8_t length)
{
	types = types + 4;
	length = length - 4;
	fprintf(stdout, "\nSupported PLDM types:");
	for (int i = 0; i < length; i++) {
		uint8_t type = types[i];

		if (i != 0) {
			continue;
		}
		for (int i = 0; i < 8; i++) {
			if (type & (1 << i)) {
				fprintf(stdout, "\nPLDM Type : %s", pldm_types[i].str);
				fprintf(stdout, "\nPLDM Type Code : %d", pldm_types[i].type);
			}
		}
	}
	fprintf(stdout, "\n");
}

bool is_transfer_flag_valid(uint8_t transfer_flag)
{
        switch (transfer_flag) {
        case PLDM_START:
        case PLDM_MIDDLE:
        case PLDM_END:
        case PLDM_START_AND_END:
                return true;

        default:
                return false;
        }
}

void buf_print(char *header, uint8_t *data, uint8_t len)
{
	char str[1024];
	int str_len = 0;
	char *s = str;
	str_len += snprintf(s + str_len, 1024 - str_len, "%s:", header);
	if (str_len >= 1024) {
		return;
	}
	for (uint8_t loop_var = 0; loop_var < len; loop_var++) {
		if (str_len < 1024) {
			if (loop_var % 0x10 == 0) {
				str_len += snprintf(s + str_len, 512 - str_len, "\n\r0x%02x",
						    data[loop_var]);
			} else {
				str_len += snprintf(s + str_len, 512 - str_len, " 0x%02x",
						    data[loop_var]);
			}
		} else {
			return;
		}
	}
	fprintf(stdout, "%s\n", str);
	s = NULL;
	str_len = 0;
}

int parse_time_date(uint8_t *ptr, timestamp104_t *timestamp)
{
    if (timestamp == NULL || ptr == NULL)
    {
        fprintf(stderr, "Invalid timestamp or AMC date time pointer\n");
        return -EINVAL;
    }
	ptr++; // Skip the first byte (time resolution and UTC resolution)

	timestamp->year = *(uint16_t *)ptr; //(timestamp->year & 0x00FF) | ((timestamp->year & 0xFF00) << 8);
    ptr += sizeof(uint16_t);            // Move pointer past year
	ptr++;
	timestamp->month = *ptr++;
    timestamp->day = *ptr++;
    timestamp->hour = *ptr++;
    timestamp->minute = *ptr++;
    timestamp->second = *ptr++;

	timestamp->microsecond = *(uint32_t *)ptr; // Read microsecond
    ptr += sizeof(uint32_t);                   // Move pointer past microsecond
   
    timestamp->utc_offset = *(uint16_t *)ptr; // Read UTC offset
    ptr += sizeof(uint16_t);                  // Move pointer past UTC offset
    
    return 0; // Success
}