#ifndef PLDM_FRU_H
#define PLDM_FRU_H

#include <stdint.h>
#include <stddef.h>

#define PLDM_FRU_FIELD_SIZE (255)
#define PLDM_FRU_MAX_RECORDS (255)
#define PLDM_MAX_FRU_RECORDS (3)

typedef enum
{
    PLDM_FRU_FIELD_TYPE_NUMERIC = 0x01,   // Numeric value
    PLDM_FRU_FIELD_TYPE_TIMESTAMP = 0x02, // Timestamp value
    PLDM_FRU_FIELD_TYPE_STRING = 0x03,    // String value
    PLDM_FRU_FIELD_TYPE_OEM = 0x04        // OEM specific value
} ptype_t;

typedef struct
{
    uint8_t major_version;
    uint8_t minor_version;
    uint32_t fru_table_maximum_size;
    uint32_t fru_table_length;
    uint16_t total_record_set_identifiers;
    uint16_t total_table_records;
    uint32_t checksum;
} pldm_fru_record_metadata_t;

typedef struct
{
    char name[32];  // Name of the FRU field
    uint8_t type;   // Type of the FRU field
    ptype_t ptype;  // PLDM field type
    uint8_t length; // Length of the FRU field value
    union
    {
        uint32_t numeric;
        timestamp104_t timestamp;            // 104-bit timestamp
        uint8_t string[PLDM_FRU_FIELD_SIZE]; // Variable length value
    };
} pldm_fru_record_field_t;

typedef struct
{
    char name[32]; // Name of the FRU record
    uint16_t record_set_id;
    uint8_t record_type;
    uint8_t encoding;
    uint8_t num_frus;
    pldm_fru_record_field_t **fru_record_field; // Variable length
} pldm_fru_record_data_format_t;

typedef struct
{
    pldm_fru_record_metadata_t metadata; // Metadata for the FRU record
    uint8_t fru_records;
    pldm_fru_record_data_format_t **record_data; // Pointer to the FRU record data
} pldm_fru_ctx_t;

int pldm_fru_init(pldm_ctx_t *ctx);
int get_fru_record_meta_data(pldm_ctx_t *ctx, pldm_fru_record_metadata_t *metadata);
void fru_info_print(pldm_ctx_t *ctx);

#endif // PLDM_FRU_H