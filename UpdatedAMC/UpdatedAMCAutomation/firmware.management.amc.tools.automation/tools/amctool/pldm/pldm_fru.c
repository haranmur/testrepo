#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include "../pldm/pldm.h"
#include "../pldm/pldm_fru.h"
#include "../libpldm/fru.h"
#include "../mctp/mctp.h"
#include "utils.h"

static int fru_record_type_to_name(uint8_t rcord_type, char *name, size_t name_len)
{
    switch (rcord_type)
    {
    case PLDM_FRU_RECORD_TYPE_GENERAL:
        snprintf(name, name_len, "General");
        break;
    case PLDM_FRU_RECORD_TYPE_OEM:
        snprintf(name, name_len, "OEM");
        break;
    default:
        snprintf(name, name_len, "Unknown");
        return -EINVAL;
    }
    return 0;
}

static int fru_field_type_to_name(uint8_t field_type, char *name, size_t name_len)
{
    switch (field_type)
    {
    case PLDM_FRU_FIELD_TYPE_CHASSIS:
        snprintf(name, name_len, "Chassis");
        break;
    case PLDM_FRU_FIELD_TYPE_MODEL:
        snprintf(name, name_len, "Model");
        break;
    case PLDM_FRU_FIELD_TYPE_PN:
        snprintf(name, name_len, "Part Number");
        break;
    case PLDM_FRU_FIELD_TYPE_SN:
        snprintf(name, name_len, "Serial Number");
        break;
    case PLDM_FRU_FIELD_TYPE_MANUFAC:
        snprintf(name, name_len, "Manufacturer");
        break;
    case PLDM_FRU_FIELD_TYPE_MANUFAC_DATE:
        snprintf(name, name_len, "Manufacture Date");
        break;
    case PLDM_FRU_FIELD_TYPE_VENDOR:
        snprintf(name, name_len, "Vendor");
        break;
    case PLDM_FRU_FIELD_TYPE_NAME:
        snprintf(name, name_len, "Name");
        break;
    case PLDM_FRU_FIELD_TYPE_SKU:
        snprintf(name, name_len, "SKU");
        break;
    case PLDM_FRU_FIELD_TYPE_VERSION:
        snprintf(name, name_len, "Version");
        break;
    case PLDM_FRU_FIELD_TYPE_ASSET_TAG:
        snprintf(name, name_len, "Asset Tag");
        break;
    case PLDM_FRU_FIELD_TYPE_DESC:
        snprintf(name, name_len, "Description");
        break;
    case PLDM_FRU_FIELD_TYPE_EC_LVL:
        snprintf(name, name_len, "EC Level");
        break;
    case PLDM_FRU_FIELD_TYPE_OTHER:
        snprintf(name, name_len, "Other");
        break;
    case PLDM_FRU_FIELD_TYPE_IANA:
        snprintf(name, name_len, "IANA Field");
        break;
    default:
        snprintf(name, name_len, "Unknown Field Type");
        return -EINVAL;
    }
    return 0;
}

enum pldm_fru_field_oem_type
{
    FIELD_TYPE_OEM_VENDOR_IANA = 1,
    FIELD_TYPE_OEM_SLAVE_ADDR = 2,
    FIELD_TYPE_OEM_SMBUS_FREQ = 3,
    FIELD_TYPE_OEM_HW_ARB_OPT_IN = 4,
};

int fru_field_oem_type_to_name(uint8_t oem_type, char *name, size_t name_len)
{
    switch (oem_type)
    {
    case FIELD_TYPE_OEM_VENDOR_IANA:
        snprintf(name, name_len, "IANA");
        break;
    case FIELD_TYPE_OEM_SLAVE_ADDR:
        snprintf(name, name_len, "Slave Addr");
        break;
    case FIELD_TYPE_OEM_SMBUS_FREQ:
        snprintf(name, name_len, "SMBus Frequency");
        break;
    case FIELD_TYPE_OEM_HW_ARB_OPT_IN:
        snprintf(name, name_len, "Hardware Arbitration Opt-In");
        break;
    default:
        snprintf(name, name_len, "Unknown OEM Type");
        return -EINVAL;
    }
    return 0;
}

int get_fru_record_meta_data(pldm_ctx_t *ctx, pldm_fru_record_metadata_t *metadata)
{
    if (ctx == NULL || metadata == NULL)
    {
        fprintf(stderr, "Invalid context or metadata pointer\n");
        return -EINVAL;
    }

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }
    uint8_t instance_id = get_instance_id();
    struct pldm_msg msg = {0};
    int rc = encode_get_fru_record_table_metadata_req(instance_id, &msg, 0);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get FRU record metadata request: %d\n", rc);
        return rc;
    }

    uint8_t buf[MCTP_MTU_SIZE];
    memset(buf, 0, MCTP_MTU_SIZE);

    struct pldm_msg *response = (struct pldm_msg *)buf;
    uint16_t response_len = sizeof(struct pldm_msg) + sizeof(*metadata);

    uint16_t req_len = 3;

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&msg, req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }
    uint8_t completion_code;

    rc = decode_get_fru_record_table_metadata_resp(response,
                                                   response_len - sizeof(struct pldm_msg_hdr),
                                                   &completion_code,
                                                   &metadata->major_version,
                                                   &metadata->minor_version,
                                                   &metadata->fru_table_maximum_size,
                                                   &metadata->fru_table_length,
                                                   &metadata->total_record_set_identifiers,
                                                   &metadata->total_table_records,
                                                   &metadata->checksum);
    if (rc)
    {
        fprintf(stderr, "Failed to decode get FRU record metadata response: %d\n", rc);
        return rc;
    }
    //fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Error in get FRU record metadata response: %d\n", completion_code);
        return -EIO;
    }
/*    
     fprintf(stdout, "FRU Record Metadata: Major Version: %d, Minor Version: %d, "
                    "Max Size: %d, Length: %d, Total Record Sets: %d, Total Records: %d\n",
            metadata->major_version, metadata->minor_version,
            metadata->fru_table_maximum_size, metadata->fru_table_length,
            metadata->total_record_set_identifiers, metadata->total_table_records);
*/            
    return 0;
}

int get_fru_record(pldm_ctx_t *ctx, uint32_t *data_transfer_handle, uint8_t *transfer_operation_flag,
                   uint8_t *record_data, size_t *record_data_length)
{
    if (ctx == NULL || record_data == NULL || record_data_length == NULL)
    {
        fprintf(stderr, "Invalid context or parameters\n");
        return -EINVAL;
    }

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);

    uint8_t instance_id = get_instance_id();
    struct pldm_msg *msg = (struct pldm_msg *)tbuf;

    uint8_t payload_length = sizeof(struct pldm_get_fru_record_table_req);

    int rc = encode_get_fru_record_table_req(instance_id,
                                             *data_transfer_handle,
                                             *transfer_operation_flag,
                                             msg,
                                             payload_length);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get FRU record request: %d\n", rc);
        return rc;
    }

    //uint8_t buf[MCTP_MTU_SIZE];
    //memset(buf, 0, MCTP_MTU_SIZE);

    uint8_t buf[512];
    memset(buf, 0, 512);

    struct pldm_msg *response = (struct pldm_msg *)buf;
    uint16_t response_len = sizeof(struct pldm_msg) + *record_data_length;

    uint16_t req_len = 3 + payload_length;

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)msg, req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code;
    uint8_t *fru_record_table_data = buf;
    size_t fru_record_table_length;

    //buf_print("PLDM GET FRU Record Response", (uint8_t *)response, response_len);

    rc = decode_get_fru_record_table_resp(response,
                                          response_len - sizeof(struct pldm_msg_hdr),
                                          &completion_code,
                                          data_transfer_handle,
                                          transfer_operation_flag,
                                          fru_record_table_data,
                                          &fru_record_table_length);
    if (rc)
    {
        fprintf(stderr, "Failed to decode get FRU record response: %d\n", rc);
        return rc;
    }

    //fprintf(stdout, "Completion Code: %d\n", completion_code);

    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Error in get FRU record response: %d\n", completion_code);
        return -EIO;
    }
/*     fprintf(stdout, "Next Data Transfer Handle: %d, Transfer Flag: %d\n",
            *data_transfer_handle, *transfer_operation_flag); */

    if (fru_record_table_length > *record_data_length)
    {
        fprintf(stderr, "FRU record data length exceeds buffer size\n");
        return -ENOMEM;
    }

/*     fprintf(stdout, "FRU Record Table Length: %zu\n", fru_record_table_length); */

    memcpy(record_data, fru_record_table_data, fru_record_table_length);
    *record_data_length = fru_record_table_length;

    return 0; // Success
}

typedef enum oem_fru_field_type
{
    PLDM_FRU_FIELD_TYPE_OEM_VENDOR_IANA = 1,
    PLDM_FRU_FIELD_TYPE_OEM_SLAVE_ADDR,
    PLDM_FRU_FIELD_TYPE_OEM_SMBUS_FREQ,
    PLDM_FRU_FIELD_TYPE_OEM_HW_ARB_OPT_IN
} oem_fru_field_type_t;

int update_oem_fru_field(pldm_ctx_t *ctx, uint8_t *field_data, pldm_fru_record_field_t *field)
{
    if (ctx == NULL || field_data == NULL || field == NULL)
    {
        fprintf(stderr, "Invalid context or parameters\n");
        return -EINVAL;
    }
    uint8_t field_type = field->type;

    oem_fru_field_type_t oem_fru_field_type = (oem_fru_field_type_t)field_type;

    switch (oem_fru_field_type)
    {
    case PLDM_FRU_FIELD_TYPE_OEM_VENDOR_IANA:
        if (field->length != sizeof(uint32_t))
        {
            fprintf(stderr, "IANA field length must be 4 bytes\n");
            return -EIO;
        }
        if (field->length > PLDM_FRU_FIELD_SIZE)
        {
            fprintf(stderr, "IANA field length exceeds maximum size\n");
            return -EIO;
        }
        field->ptype = PLDM_FRU_FIELD_TYPE_NUMERIC; // IANA field is numeric
        field->numeric = *(uint32_t *)field_data;   // Store the IANA value
        if (fru_field_oem_type_to_name(field->type, field->name, sizeof(field->name)) < 0)
        {
            fprintf(stderr, "Failed to get OEM field name for type: %d\n", field->type);
            return -EINVAL;
        }
        break;
    case PLDM_FRU_FIELD_TYPE_OEM_SLAVE_ADDR:
        if (field->length != sizeof(uint8_t))
        {
            fprintf(stderr, "Slave address field length must be 1 byte\n");
            return -EIO;
        }
        if (field->length > PLDM_FRU_FIELD_SIZE)
        {
            fprintf(stderr, "Slave address field length exceeds maximum size\n");
            return -EIO;
        }
        field->ptype = PLDM_FRU_FIELD_TYPE_NUMERIC; // Slave address is numeric
        field->numeric = *field_data;               // Store the slave address
        if (fru_field_oem_type_to_name(field->type, field->name, sizeof(field->name)) < 0)
        {
            fprintf(stderr, "Failed to get OEM field name for type: %d\n", field->type);
            return -EINVAL;
        }
        break;
    case PLDM_FRU_FIELD_TYPE_OEM_SMBUS_FREQ:
        if (field->length != sizeof(uint16_t))
        {
            fprintf(stderr, "SMBus frequency field length must be 4 bytes\n");
            return -EIO;
        }
        if (field->length > PLDM_FRU_FIELD_SIZE)
        {
            fprintf(stderr, "SMBus frequency field length exceeds maximum size\n");
            return -EIO;
        }
        field->ptype = PLDM_FRU_FIELD_TYPE_NUMERIC; // SMBus frequency is numeric
        field->numeric = *(uint16_t *)field_data;   // Store the SMBus frequency
        if (fru_field_oem_type_to_name(field->type, field->name, sizeof(field->name)) < 0)
        {
            fprintf(stderr, "Failed to get OEM field name for type: %d\n", field->type);
            return -EINVAL;
        }
        break;
    case PLDM_FRU_FIELD_TYPE_OEM_HW_ARB_OPT_IN:
        if (field->length != sizeof(uint16_t))
        {
            fprintf(stderr, "Hardware arbitration opt-in field length must be 1 byte\n");
            return -EIO;
        }
        if (field->length > PLDM_FRU_FIELD_SIZE)
        {
            fprintf(stderr, "Hardware arbitration opt-in field length exceeds maximum size\n");
            return -EIO;
        }
        field->ptype = PLDM_FRU_FIELD_TYPE_NUMERIC; // Hardware arbitration opt-in is numeric
        field->numeric = *(uint16_t *)field_data;               // Store the hardware arbitration opt-in value
        if (fru_field_oem_type_to_name(field->type, field->name, sizeof(field->name)) < 0)
        {
            fprintf(stderr, "Failed to get OEM field name for type: %d\n", field->type);
            return -EINVAL;
        }
        break;
    default:
        fprintf(stderr, "Unknown OEM FRU field type: %d\n", oem_fru_field_type);
        return -EINVAL;
    }
    return 0; // Success
}

int update_general_fru_field(pldm_ctx_t *ctx, uint8_t *field_data, pldm_fru_record_field_t *field)
{
    if (ctx == NULL || field_data == NULL || field == NULL)
    {
        fprintf(stderr, "Invalid context or record data pointer\n");
        return -EINVAL;
    }

    pldm_fru_ctx_t *fru_ctx = ctx->fru_ctx;
    if (fru_ctx == NULL)
    {
        fprintf(stderr, "FRU context is not initialized\n");
        return -EINVAL;
    }

    uint8_t field_type = field->type;

    switch (field_type)
    {
    case PLDM_FRU_FIELD_TYPE_CHASSIS:
    case PLDM_FRU_FIELD_TYPE_MODEL:
    case PLDM_FRU_FIELD_TYPE_PN:
    case PLDM_FRU_FIELD_TYPE_SN:
    case PLDM_FRU_FIELD_TYPE_MANUFAC:
    case PLDM_FRU_FIELD_TYPE_VENDOR:
    case PLDM_FRU_FIELD_TYPE_NAME:
    case PLDM_FRU_FIELD_TYPE_SKU:
    case PLDM_FRU_FIELD_TYPE_VERSION:
    case PLDM_FRU_FIELD_TYPE_ASSET_TAG:
    case PLDM_FRU_FIELD_TYPE_DESC:
    case PLDM_FRU_FIELD_TYPE_EC_LVL:
    case PLDM_FRU_FIELD_TYPE_OTHER:
        if (field->length > PLDM_FRU_FIELD_SIZE)
        {
            fprintf(stderr, "FRU field length exceeds maximum size\n");
            return -EIO;
        }
        memcpy(field->string, field_data, field->length);
        field->string[field->length] = '\0';       // Null-terminate the string
        field->ptype = PLDM_FRU_FIELD_TYPE_STRING; // Set the PLDM field type to string
        break;
    case PLDM_FRU_FIELD_TYPE_MANUFAC_DATE:
        if (field->length != sizeof(timestamp104_t))
        {
            fprintf(stderr, "Manufacture date field length must be 4 bytes\n");
            return -EIO;
        }
        if (field->length > PLDM_FRU_FIELD_SIZE)
        {
            fprintf(stderr, "Manufacture date field length exceeds maximum size\n");
            return -EIO;
        }
        parse_time_date(field_data, &field->timestamp);
        field->ptype = PLDM_FRU_FIELD_TYPE_TIMESTAMP; // Set the PLDM field type to timestamp
        break;
    case PLDM_FRU_FIELD_TYPE_IANA:
        if (field->length != sizeof(uint32_t))
        {
            fprintf(stderr, "IANA field length must be 4 bytes\n");
            return -EIO;
        }
        if (field->length > PLDM_FRU_FIELD_SIZE)
        {
            fprintf(stderr, "IANA field length exceeds maximum size\n");
            return -EIO;
        }
        field->ptype = PLDM_FRU_FIELD_TYPE_NUMERIC; // IANA field is numeric
        field->numeric = *(uint32_t *)field_data;
        break;
    default:
        fprintf(stderr, "Unknown FRU field type: %d\n", field_type);
        return -EINVAL;
    }
    fru_field_type_to_name(field->type, field->name, sizeof(field->name));
    return 0; // Success
}

int update_fru_field(pldm_ctx_t *ctx, uint8_t record_type, uint8_t *field_data, pldm_fru_record_field_t *field)
{
    if (ctx == NULL || field_data == NULL || field == NULL)
    {
        fprintf(stderr, "Invalid context or parameters\n");
        return -EINVAL;
    }
    uint8_t field_type = field->type;

    if (record_type == PLDM_FRU_RECORD_TYPE_OEM)
    {
        if (field_type < PLDM_FRU_FIELD_TYPE_OEM_VENDOR_IANA ||
            field_type > PLDM_FRU_FIELD_TYPE_OEM_HW_ARB_OPT_IN)
        {
            fprintf(stderr, "Invalid OEM FRU field type: %d\n", field_type);
            return -EINVAL;
        }
        return update_oem_fru_field(ctx, field_data, field);
    }
    else
    {
        if (field_type < PLDM_FRU_FIELD_TYPE_CHASSIS || field_type > PLDM_FRU_FIELD_TYPE_IANA)
        {
            fprintf(stderr, "Invalid general FRU field type: %d\n", field_type);
            return -EINVAL;
        }
        return update_general_fru_field(ctx, field_data, field);
    }
    return 0;
}

int parse_fru_record_data(pldm_ctx_t *ctx, uint8_t *record_data, uint16_t total_table_length)
{
    if (ctx == NULL || record_data == NULL)
    {
        fprintf(stderr, "Invalid context or record data pointer\n");
        return -EINVAL;
    }

    pldm_fru_ctx_t *fru_ctx = ctx->fru_ctx;
    if (fru_ctx == NULL)
    {
        fprintf(stderr, "FRU context is not initialized\n");
        return -EINVAL;
    }

    if (total_table_length > fru_ctx->metadata.fru_table_maximum_size)
    {
        fprintf(stderr, "Total table length exceeds maximum size\n");
        return -EIO;
    }

    fru_ctx->record_data = malloc(sizeof(fru_ctx->record_data) * fru_ctx->metadata.total_table_records);
    if (fru_ctx->record_data == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for FRU record data\n");
        return -ENOMEM;
    }
    memset(fru_ctx->record_data, 0, sizeof(fru_ctx->record_data) * fru_ctx->metadata.total_table_records);
    // Parse the record data
    uint8_t *ptr = record_data;

    pldm_fru_record_data_format_t *record;

    for (uint16_t i = 0; i < fru_ctx->metadata.total_table_records; i++)
    {
        record = malloc(sizeof(pldm_fru_record_data_format_t));
        if (record == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for FRU record %d\n", i);
            free(fru_ctx->record_data);
            return -ENOMEM;
        }
        record->record_set_id = *(uint16_t *)ptr;
        ptr += sizeof(uint16_t);

        record->record_type = *ptr++;
        fru_record_type_to_name(record->record_type, record->name, sizeof(record->name));

        // Read the number of FRUs
        record->num_frus = *ptr++;
        record->encoding = *ptr++;

        // Allocate memory for FRU fields
        record->fru_record_field = malloc(sizeof(record->fru_record_field) * record->num_frus);
        if (record->fru_record_field == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for FRU fields in record %d\n", i);
            free(record);
            return -ENOMEM;
        }
        pldm_fru_record_field_t *field;

        // Read each FRU field
        for (uint8_t j = 0; j < record->num_frus; j++)
        {
            field = malloc(sizeof(pldm_fru_record_field_t));
            if (field == NULL)
            {
                fprintf(stderr, "Failed to allocate memory for FRU field %d in record %d\n", j, i);
                free(record->fru_record_field);
                free(record);
                return -ENOMEM;
            }

            field->type = *ptr++;
            field->length = *ptr++;
            if (field->length > PLDM_FRU_FIELD_SIZE)
            {
                fprintf(stderr, "FRU field length exceeds maximum size\n");
                free(record->fru_record_field);
                free(record);
                return -EIO;
            }

            int rc = update_fru_field(ctx, record->record_type, ptr, field);
            if (rc)
            {
                fprintf(stderr, "Unknown FRU field type: %d\n", field->type);
                free(record->fru_record_field);
                free(record);
                return -EIO;
            }
            ptr += field->length; // Move pointer to the next field
            record->fru_record_field[j] = field;
        }
        fru_ctx->record_data[i] = record;
    }
    return 0; // Success
}

void fru_info_print(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->fru_ctx == NULL)
    {
        fprintf(stderr, "Invalid context or FRU context\n");
        return;
    }

    pldm_fru_ctx_t *fru_ctx = ctx->fru_ctx;
    fprintf(stdout, "FRU Record Metadata:\n");
    fprintf(stdout, "Major Version: %d\n", fru_ctx->metadata.major_version);
    fprintf(stdout, "Minor Version: %d\n", fru_ctx->metadata.minor_version);
    fprintf(stdout, "Max Size: %d\n", fru_ctx->metadata.fru_table_maximum_size);
    fprintf(stdout, "Length: %d\n", fru_ctx->metadata.fru_table_length);
    fprintf(stdout, "Total Record Sets: %d\n", fru_ctx->metadata.total_record_set_identifiers);
    fprintf(stdout, "Total Records: %d\n", fru_ctx->metadata.total_table_records);

    for (uint16_t i = 0; i < fru_ctx->metadata.total_table_records; i++)
    {
        pldm_fru_record_data_format_t *record = fru_ctx->record_data[i];
        if (record == NULL)
        {
            continue; // Skip if record is NULL
        }
        fprintf(stdout, "Record Set ID: 0x%x\n", record->record_set_id);
        fprintf(stdout, "Record Type: %s (%d)\n", record->name, record->record_type);
        fprintf(stdout, "Number of FRUs: %d\n", record->num_frus);
        fprintf(stdout, "Encoding: %d\n", record->encoding);

        for (uint8_t j = 0; j < record->num_frus; j++)
        {
            pldm_fru_record_field_t *field = record->fru_record_field[j];
            if (field == NULL)
            {
                continue; // Skip if field is NULL
            }
            if (field->ptype == PLDM_FRU_FIELD_TYPE_NUMERIC)
            {
                fprintf(stdout, "%d: \t(%02d) %s  \t: 0x%x\n", j + 1, field->type, field->name, field->numeric);
                continue;
            }
            if (field->ptype == PLDM_FRU_FIELD_TYPE_STRING)
            {
                fprintf(stdout, "%d: \t(%02d) %s \t: %s\n", j + 1, field->type, field->name, field->string);
                continue;
            }
            if (field->ptype == PLDM_FRU_FIELD_TYPE_OEM)
            {
                char oem_name[32];
                fru_field_oem_type_to_name(field->type, oem_name, sizeof(oem_name));
                fprintf(stdout, "%d: \t(%02d) %s  \t: OEM Value: %s\n", j + 1, field->type, field->name, oem_name);
                continue;
            }
            if (field->ptype == PLDM_FRU_FIELD_TYPE_TIMESTAMP)
            {
                fprintf(stdout, "%d: \t(%02d) %s : %04d-%02d-%02d %02d:%02d:%02d.%06d UTC Offset: %d\n",
                        j + 1, field->type, field->name, 
                        field->timestamp.year, field->timestamp.month, field->timestamp.day,
                        field->timestamp.hour, field->timestamp.minute, field->timestamp.second,
                        field->timestamp.microsecond, field->timestamp.utc_offset);
                continue;
            }
        }
        fprintf(stdout, "\n");
    }
}

int pldm_fru_init(pldm_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        fprintf(stderr, "PLDM is not initialized\n");
        return -EINVAL;
    }

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "MCTP is not initialized\n");
        return -EINVAL;
    }

    pldm_fru_ctx_t *fru_ctx = malloc(sizeof(pldm_fru_ctx_t));
    if (fru_ctx == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for FRU context\n");
        return -ENOMEM;
    }
    memset(fru_ctx, 0, sizeof(pldm_fru_ctx_t));

    ctx->fru_ctx = fru_ctx;

    int rc = get_fru_record_meta_data(ctx, &fru_ctx->metadata);
    if (rc)
    {
        fprintf(stderr, "Failed to get FRU record metadata: %d\n", rc);
        free(ctx->fru_ctx);
        ctx->fru_ctx = NULL;
        return rc;
    }
    if (fru_ctx->metadata.major_version == 0 && fru_ctx->metadata.minor_version == 0)
    {
        fprintf(stderr, "Failed to retrieve FRU record metadata\n");
        return -EIO;
    }

    uint32_t data_transfer_handle = 0;
    uint8_t transfer_operation_flag = PLDM_GET_FIRSTPART;
    uint8_t record_data[fru_ctx->metadata.fru_table_maximum_size];
    size_t record_data_length = fru_ctx->metadata.fru_table_maximum_size;
    memset(record_data, 0, fru_ctx->metadata.fru_table_maximum_size);

    uint8_t *rptr = record_data;
    uint8_t total_table_length = 0;

    while (true)
    {
        rc = get_fru_record(ctx, &data_transfer_handle,
                            &transfer_operation_flag,
                            rptr,
                            &record_data_length);
        if (rc)
        {
            fprintf(stderr, "Failed to get FRU record: %d\n", rc);
            free(ctx->fru_ctx);
            ctx->fru_ctx = NULL;
            return rc;
        }
        rptr += record_data_length;
        if (record_data_length == 0)
        {
            fprintf(stderr, "No more FRU records available\n");
            break; // No more records to process
        }
        total_table_length += record_data_length;
        if (transfer_operation_flag == PLDM_END)
        {
            break; // Last part received, exit loop
        }
        transfer_operation_flag = PLDM_GET_NEXTPART;
    }

    rc = parse_fru_record_data(ctx, record_data, total_table_length);
    if (rc)
    {
        fprintf(stderr, "Failed to parse FRU record data: %d\n", rc);
        free(ctx->fru_ctx);
        ctx->fru_ctx = NULL;
        return rc;
    }
    return rc; // Success
}
