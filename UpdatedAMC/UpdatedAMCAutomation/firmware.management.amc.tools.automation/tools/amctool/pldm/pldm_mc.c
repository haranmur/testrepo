#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <wchar.h>
#include <locale.h>
#include "../libpldm/base.h"
#include "config.h"
#include "pldm.h"
#include "../mctp/mctp.h"
#include "../libpldm/platform.h"
#include "../libpldm/pdr.h"
#include "utils.h"

#define PLDM_MAX_PDR_RECORD_SIZE 4096
#define ENTITY_TYPE_MASK 0x7F
#define ENTITY_TYPE_SHIFT 7

const uint8_t pdr_type_values[] = {
    PLDM_TERMINUS_LOCATOR_PDR,
    PLDM_NUMERIC_SENSOR_PDR,
    PLDM_STATE_SENSOR_PDR,
    PLDM_SENSOR_AUXILIARY_NAMES_PDR,
    PLDM_NUMERIC_EFFECTER_PDR,
    PLDM_STATE_EFFECTER_PDR,
    PLDM_EFFECTER_AUXILIARY_NAMES_PDR,
    PLDM_PDR_ENTITY_ASSOCIATION,
    PLDM_ENTITY_AUXILIARY_NAMES_PDR,
    PLDM_PDR_FRU_RECORD_SET,
};

static char *pdr_type_to_string(uint8_t pdr_type)
{
    switch (pdr_type)
    {
    case PLDM_TERMINUS_LOCATOR_PDR:
        return "Terminus Locator PDR";
    case PLDM_NUMERIC_SENSOR_PDR:
        return "Numeric Sensor PDR";
    case PLDM_STATE_SENSOR_PDR:
        return "State Sensor PDR";
    case PLDM_SENSOR_AUXILIARY_NAMES_PDR:
        return "Sensor Auxiliary Names PDR";
    case PLDM_NUMERIC_EFFECTER_PDR:
        return "Numeric Effecter PDR";
    case PLDM_STATE_EFFECTER_PDR:
        return "State Effecter PDR";
    case PLDM_EFFECTER_AUXILIARY_NAMES_PDR:
        return "Effecter Auxiliary Names PDR";
    case PLDM_PDR_ENTITY_ASSOCIATION:
        return "PDR Entity Association";
    case PLDM_ENTITY_AUXILIARY_NAMES_PDR:
        return "Entity Auxiliary Names PDR";
    case PLDM_PDR_FRU_RECORD_SET:
        return "PDR FRU Record Set";
    default:
        return "Unknown PDR Type";
    }
}
#define PLDM_GET_TERMINUS_UID_REQ_BYTES 0
#define PLDM_GET_TERMINUS_UID_RESP_BYTES 17

static int get_terminus_uid(pldm_ctx_t *ctx)
{
	if (ctx == NULL || ctx->platform_ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or platform context\n");
        return -EINVAL;
    }

    uint8_t *terminus_uid = ctx->platform_ctx->terminus_uid;

	struct pldm_msg msg;

    uint8_t buf[PLDM_MAX_CMDS_PER_TYPE];
    memset(buf, 0, PLDM_MAX_CMDS_PER_TYPE);

	uint16_t req_len = sizeof (struct pldm_msg_hdr) + PLDM_GET_TERMINUS_UID_REQ_BYTES;

	uint8_t instance_id = get_instance_id();

    int rc = encode_header_only_request(instance_id, PLDM_PLATFORM,
					  PLDM_GET_TERMINUS_UID, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get terminus UID request: %d\n", rc);
        return rc;
    }

	struct pldm_msg *response = (struct pldm_msg *)buf;
	uint16_t response_len = sizeof (struct pldm_msg_hdr) + PLDM_GET_TERMINUS_UID_RESP_BYTES;

	 rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&msg,
                                      req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }
	uint8_t cc;
	rc = decode_get_terminus_uid_resp(response, PLDM_GET_TERMINUS_UID_RESP_BYTES, &cc, terminus_uid);
    if (rc)
    {
        fprintf(stderr, "Failed to decode get terminus UID response: %d\n", rc);
        return rc;
    }

    if (cc != PLDM_SUCCESS)
    {
        fprintf(stderr, "Get terminus UID failed with completion code: %d\n", cc);
        return -EIO;
    }

	return rc;
}

static void pdr_repository_info_print(struct pldm_pdr_repository_info *info)
{
    if (info == NULL)
    {
        fprintf(stderr, "Invalid PDR repository info pointer\n");
        return;
    }

    timestamp104_t timestamp = {0};

    fprintf(stdout, "\n\nPDR Repository Info:\n");
    fprintf(stdout, "Repository State: %d\n", info->repository_state);
    parse_time_date((uint8_t *)&info->update_time, &timestamp);
    fprintf(stdout, "Update Time: %04d-%02d-%02d %02d:%02d:%02d.%06d UTC Offset: %d minutes\n",
           timestamp.year, timestamp.month, timestamp.day,
           timestamp.hour, timestamp.minute, timestamp.second,
           timestamp.microsecond, timestamp.utc_offset);
    parse_time_date((uint8_t *)&info->oem_update_time, &timestamp);
    fprintf(stdout, "OEM Update Time: %04d-%02d-%02d %02d:%02d:%02d.%06d UTC Offset: %d minutes\n",
              timestamp.year, timestamp.month, timestamp.day,
              timestamp.hour, timestamp.minute, timestamp.second,
              timestamp.microsecond, timestamp.utc_offset);
    fprintf(stdout, "Record Count: %u\n", info->record_count);
    fprintf(stdout, "Repository Size: %u\n", info->repository_size);
    fprintf(stdout, "Largest Record Size: %u\n", info->largest_record_size);
    fprintf(stdout, "Data Transfer Handle Timeout: %u\n", info->data_transfer_handle_timeout);
    fprintf(stdout, "\n");
}

/**
 * @brief Retrieves PDR repository information from the PLDM device.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
static int get_pdr_repository_info(pldm_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context\n");
        return -EINVAL;
    }

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    struct pldm_msg msg = {0};
    int rc = encode_get_pdr_repository_info_req(ctx->tid, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get PDR repository info request: %d\n", rc);
        return rc;
    }

    uint8_t buf[PLDM_MAX_CMDS_PER_TYPE];
    memset(buf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)buf;
    uint16_t req_len = 3;
    uint16_t response_len = 72;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&msg,
                                      req_len,
                                      (uint8_t *)response, &response_len);

    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    struct pldm_get_pdr_repository_info_resp resp = {0};

    rc = decode_get_pdr_repository_info_resp(
        response,
        response_len - sizeof(struct pldm_msg_hdr),
        &resp);
    if (rc)
    {
        fprintf(stderr, "Failed to decode get PDR repository info response: %d\n", rc);
        return rc;
    }

    //buf_print("PLDM GET PDR Repository Info Response", (uint8_t *)response, response_len);

    if (resp.completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Get PDR repository info failed with completion code: %d\n", resp.completion_code);
        return -EIO;
    }

    memcpy(&ctx->platform_ctx->pdr_repository_info, &resp.pdr_repo_info, sizeof(struct pldm_pdr_repository_info));
    //pdr_repository_info_print(&ctx->platform_ctx->pdr_repository_info);

    if (resp.pdr_repo_info.repository_state != PLDM_PDR_REPOSITORY_STATE_AVAILABLE)
    {
        fprintf(stderr, "PDR repository is not available\n");
        return -EIO;
    }
    if (resp.pdr_repo_info.record_count == 0)
    {
        fprintf(stderr, "No records found in PDR repository\n");
        return -EIO;
    }
    return 0; // Success
}

/**
 * @brief Retrieves all PDR records from the PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */

int get_all_pdrs(pldm_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context\n");
        return -EINVAL;
    }

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    ctx->platform_ctx->pdrs = pldm_pdr_init();
    if (ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Failed to initialize PDR repository\n");
        free(ctx->platform_ctx);
        ctx->platform_ctx = NULL;
        return -ENOMEM;
    }

    uint32_t record_handle = 0;
    uint32_t data_transfer_handle = 0;
    uint8_t transfer_op_flag = PLDM_GET_FIRSTPART;
    uint8_t transfer_flag = 0;
    uint16_t request_count = ctx->platform_ctx->pdr_repository_info.largest_record_size;
    uint16_t record_chg_num = 0;
    uint16_t resp_count = 0;

    uint8_t tbuf[MCTP_MTU_SIZE];
    struct pldm_msg *request = (struct pldm_msg *)tbuf;

    uint8_t rbuf[MCTP_MTU_SIZE];
    struct pldm_msg *response = (struct pldm_msg *)rbuf;

    uint16_t req_len = PLDM_GET_PDR_REQ_BYTES;
    uint16_t response_len = MCTP_MTU_SIZE; // ctx->platform_ctx->pdr_repository_info.largest_record_size + sizeof(struct pldm_msg_hdr) + PLDM_GET_PDR_MIN_RESP_BYTES;
    uint8_t completion_code;
    uint8_t transfer_crc = 0;
    uint8_t record_data[ctx->platform_ctx->pdr_repository_info.largest_record_size];
    size_t record_data_length;
    uint8_t *record_data_ptr;
    int record = 0;

    for (; record < ctx->platform_ctx->pdr_repository_info.record_count; record++)
    {
        memset(tbuf, 0, MCTP_MTU_SIZE);
        memset(rbuf, 0, MCTP_MTU_SIZE);
        memset(record_data, 0, sizeof(record_data));

        record_data_ptr = record_data;
        record_data_length = sizeof(record_data);
        transfer_op_flag = PLDM_GET_FIRSTPART;
        request_count = ctx->platform_ctx->pdr_repository_info.largest_record_size;
        record_chg_num = 0;
        data_transfer_handle = 0;

        while (true)
        {
            uint8_t instance_id = get_instance_id();
            req_len = PLDM_GET_PDR_REQ_BYTES;

            int rc = encode_get_pdr_req(instance_id, record_handle, data_transfer_handle,
                                        transfer_op_flag, request_count, record_chg_num,
                                        request, req_len);

            if (rc)
            {
                fprintf(stderr, "Failed to encode get PDR request: %d\n", rc);
                return rc;
            }

            req_len += sizeof(struct pldm_msg_hdr);

            //buf_print("PLDM GET PDR Request", (uint8_t *)request, req_len);

            rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx,
                                              ctx->tid,
                                              (uint8_t *)request,
                                              req_len,
                                              (uint8_t *)response,
                                              &response_len);
            if (rc)
            {
                fprintf(stderr, "Failed to send/receive message: %d\n", rc);
                return rc;
            }

            // buf_print("PLDM GET PDR Response", (uint8_t *)response, response_len);

            if (response_len < sizeof(struct pldm_msg_hdr) + PLDM_GET_PDR_MIN_RESP_BYTES)
            {
                fprintf(stderr, "Invalid response length: %d\n", response_len);
                return -EIO;
            }

            rc = decode_get_pdr_resp(response, response_len - sizeof(struct pldm_msg_hdr),
                                     &completion_code, &record_handle,
                                     &data_transfer_handle,
                                     &transfer_flag, &resp_count,
                                     record_data_ptr, record_data_length,
                                     &transfer_crc);
            if (rc)
            {
                fprintf(stderr, "Failed to encode get PDR request: %d\n", rc);
                return rc;
            }
            if (completion_code != PLDM_SUCCESS)
            {
                fprintf(stderr, "Get PDR failed with completion code: %d\n", completion_code);
                return -EIO;
            }

            if (transfer_flag == PLDM_END || transfer_flag == PLDM_START_AND_END)
            {
                record_data_length -= resp_count;
                record_data_ptr += resp_count;
                break;
            }
            transfer_op_flag = PLDM_GET_NEXTPART;
            record_data_length -= resp_count;
            record_data_ptr += resp_count;
        }
        size_t record_size = record_data_ptr - record_data;
        // fprintf(stdout, "PDR Record %d Size: %zu bytes\n", record, record_size);

        if (record_size <= 0)
        {
            fprintf(stderr, "Invalid PDR record size: %zu bytes\n", record_size);
            buf_print("PLDM GET PDR Request", (uint8_t *)request, req_len);
            buf_print("PLDM GET PDR Response", (uint8_t *)response, response_len);
        }

        if (record_size > ctx->platform_ctx->pdr_repository_info.largest_record_size)
        {
            fprintf(stderr, "PDR record size exceeds maximum limit: %zu\n", record_size);
            return -EIO;
        }

        if (record_size == 0)
        {
            fprintf(stderr, "PDR record size is zero- %u\n", record);
            return -EIO;
        }
        uint32_t handle = pldm_pdr_add(ctx->platform_ctx->pdrs, record_data, record_size, record_handle - 1, true);
        if (handle != record_handle - 1)
        {
            fprintf(stderr, "Failed to add new PDR record\n");
            return -ENOMEM;
        }
    }
    if (record == ctx->platform_ctx->pdr_repository_info.record_count)
    {
        //fprintf(stdout, "All PDR records retrieved successfully\n");
    }
    else
    {
        fprintf(stderr, "Failed to retrieve all PDR records\n");
        return -EIO;
    }
    return 0; // Success
}

/* @brief Retrieves the number of PDR records of a specific type from the PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @param pdr_type The type of PDR records to count.
 * @return The number of PDR records of the specified type, or negative error code on failure.
 */
int get_num_pdrs_by_type(pldm_ctx_t *ctx, uint8_t pdr_type)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }

    uint8_t *data = NULL;
    uint32_t size;

    const struct pldm_pdr_record *record = NULL;

    int record_count = ctx->platform_ctx->pdr_repository_info.record_count; // Count the first record
    int count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(ctx->platform_ctx->pdrs, pdr_type, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        count++;
    }

    return count; // Return the number of PDR records found
}

void terminus_locator_pdr_print(struct pldm_terminus_locator_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid Terminus Locator PDR\n");
        return;
    }

    fprintf(stdout, "Terminus Locator PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "Terminus Locator Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Locator Validity: %d\n",
            pdr->validity);
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->terminus_handle);
    fprintf(stdout, "Terminus Locator PDR Record Change Number: 0x%x\n",
            pdr->hdr.record_change_num);
    fprintf(stdout, "Terminus Locator PDR Length: %u\n",
            pdr->hdr.length);
    fprintf(stdout, "Terminus Locator PDR Version: 0x%x\n",
            pdr->hdr.version);
    fprintf(stdout, "Terminus Locator TID: 0x%x\n",
            pdr->tid);
    fprintf(stdout, "Terminus Locator Container ID: 0x%x\n",
            pdr->container_id);
    fprintf(stdout, "Terminus Locator Type: %d\n",
            pdr->terminus_locator_type);
    fprintf(stdout, "Terminus Locator Value Size: %d\n",
            pdr->terminus_locator_value_size);
    fprintf(stdout, "Terminus Locator Value: ");
    for (int i = 0; i < pdr->terminus_locator_value_size; i++)
    {
        fprintf(stdout, "%02x ", pdr->terminus_locator_value[i]);
    }
    fprintf(stdout, "\n");
}

/* * @brief Parses PDR from the PDR Repo from PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int parse_terminus_locator_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }

    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;

    int count = get_num_pdrs_by_type(ctx, PLDM_TERMINUS_LOCATOR_PDR);
    platform_ctx->terminus_locator_record_count = count;

    platform_ctx->terminus_locator_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->terminus_locator_pdr == NULL)
    {
        fprintf(stderr, "Failed to find Terminus Locate PDR\n");
        free(platform_ctx->terminus_locator_pdr);
        return -ENOENT;
    }

    const struct pldm_pdr_record *record = NULL;

    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_TERMINUS_LOCATOR_PDR, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->terminus_locator_pdr[count] = malloc(size);
        if (platform_ctx->terminus_locator_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for Terminus Locator PDR\n");
            free(platform_ctx->terminus_locator_pdr);
            platform_ctx->terminus_locator_pdr = NULL;
            return -ENOMEM;
        }
        memcpy(platform_ctx->terminus_locator_pdr[count], data, size);
        count++;
    }

    return PLDM_SUCCESS; // Success
}


void numeric_sensor_pdr_print(struct pldm_numeric_sensor_value_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid Numeric Sensor PDR\n");
        return;
    }

    fprintf(stdout, "\nNumeric Sensor PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "Numeric Sensor Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->terminus_handle);
    fprintf(stdout, "Sensor ID: 0x%x\n",
            pdr->sensor_id);
    fprintf(stdout, "Entity Type: 0x%x\n",
            pdr->entity_type & ENTITY_TYPE_MASK);
    fprintf(stdout, "Physical Entity : %d\n", (pdr->entity_type & ENTITY_TYPE_MASK) >> ENTITY_TYPE_SHIFT);
            
    fprintf(stdout, "Entity Instance: 0x%x\n",
            pdr->entity_instance_num);
    fprintf(stdout, "Container ID: 0x%x\n",
            pdr->container_id);
    fprintf(stdout, "Sensor Init: %d\n",
            pdr->sensor_init);
    fprintf(stdout, "Sensor Auxiliary Names PDR: %d\n",
            pdr->sensor_auxiliary_names_pdr);
    fprintf(stdout, "Base Unit: %d\n",
            pdr->base_unit);
    fprintf(stdout, "Unit Modifier: %d\n",
            pdr->unit_modifier);
    fprintf(stdout, "Rate Unit: %d\n",
            pdr->rate_unit);
    fprintf(stdout, "Base OEM Unit Handle: %d\n",
            pdr->base_oem_unit_handle);
    fprintf(stdout, "Auxiliary Unit: %d\n",
            pdr->aux_unit);
    fprintf(stdout, "Auxiliary Unit Modifier: %d\n",
            pdr->aux_unit_modifier);
    fprintf(stdout, "Auxiliary Rate Unit: %d\n",
            pdr->auxrate_unit);
    fprintf(stdout, "Relative: %d\n",
            pdr->rel);
    fprintf(stdout, "Auxiliary OEM Unit Handle: %d\n",
            pdr->aux_oem_unit_handle);
    fprintf(stdout, "Is Linear: %d\n",
            pdr->is_linear);
    fprintf(stdout, "Sensor Data Size: %d\n",
            pdr->sensor_data_size);
    fprintf(stdout, "Resolution: %f\n",
            pdr->resolution);
    fprintf(stdout, "Offset: %f\n",
            pdr->offset);
    fprintf(stdout, "Accuracy: %d\n",
            pdr->accuracy);
    fprintf(stdout, "Plus Tolerance: %d\n",
            pdr->plus_tolerance);
    fprintf(stdout, "Minus Tolerance: %d\n",
            pdr->minus_tolerance);
    fprintf(stdout, "Hysteresis: %d\n",
            pdr->hysteresis.value_u32);
    fprintf(stdout, "Supported Thresholds: 0x%x\n",
            pdr->supported_thresholds.byte);
    fprintf(stdout, "Threshold and Hysteresis Volatility: 0x%x\n",
            pdr->threshold_and_hysteresis_volatility.byte);
    fprintf(stdout, "State Transition Interval: %f\n",
            pdr->state_transition_interval);
    fprintf(stdout, "Update Interval: %f\n",
            pdr->update_interval);
    fprintf(stdout, "Max Readable: %d\n",
            pdr->max_readable.value_u32);
    fprintf(stdout, "Min Readable: %d\n",
            pdr->min_readable.value_u32);
    fprintf(stdout, "Range Field Format: %d\n",
            pdr->range_field_format);
    fprintf(stdout, "Range Field Support: 0x%x\n",
            pdr->range_field_support.byte);
    fprintf(stdout, "Nominal Value: %d\n",
            pdr->nominal_value.value_u32);
    fprintf(stdout, "Normal Max: %d\n",
            pdr->normal_max.value_u32);
    fprintf(stdout, "Normal Min: %d\n",
            pdr->normal_min.value_u32);
    fprintf(stdout, "Warning High: %d\n",
            pdr->warning_high.value_u32);
    fprintf(stdout, "Warning Low: %d\n",
            pdr->warning_low.value_u32);
    fprintf(stdout, "Critical High: %d\n",
            pdr->critical_high.value_u32);
    fprintf(stdout, "Critical Low: %d\n",
            pdr->critical_low.value_u32);
    fprintf(stdout, "Fatal High: %d\n",
            pdr->fatal_high.value_u32);
    fprintf(stdout, "Fatal Low: %d\n",
            pdr->fatal_low.value_u32);
    fprintf(stdout, "\n"); 
}

/** @brief Prints the Numeric Sensor PDR record.
 * @param pdr Pointer to the Numeric Sensor PDR record.
 *  @return 0 on success, negative error code on failure
 */
int parse_numeric_sensor_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_NUMERIC_SENSOR_PDR);
    platform_ctx->numeric_sensor_record_count = count;

    platform_ctx->numeric_sensor_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->numeric_sensor_pdr == NULL)
    {
        fprintf(stderr, "Failed to find Numeric Sensor PDR\n");
        free(platform_ctx->numeric_sensor_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_NUMERIC_SENSOR_PDR, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->numeric_sensor_pdr[count] = (struct pldm_numeric_sensor_value_pdr *)malloc(size);
        if (platform_ctx->numeric_sensor_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for Numeric Sensor PDR\n");
            free(platform_ctx->numeric_sensor_pdr);
            platform_ctx->numeric_sensor_pdr = NULL;
            return -ENOMEM;
        }
        pldm_numeric_sensor_pdr_parse((uint8_t *)data, size, (uint8_t *)platform_ctx->numeric_sensor_pdr[count]);
        count++;
    }
    return PLDM_SUCCESS; // Success
}

void state_sensor_pdr_print(struct pldm_state_sensor_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid State Sensor PDR\n");
        return;
    }

    fprintf(stdout, "State Sensor PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "State Sensor Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->terminus_handle);
    fprintf(stdout, "Sensor ID: 0x%x\n",
            pdr->sensor_id);
    fprintf(stdout, "Entity Type: 0x%x\n",
            pdr->entity_type & ENTITY_TYPE_MASK);
    fprintf(stdout, "Physical Entity : %d\n", (pdr->entity_type & ENTITY_TYPE_MASK) >> ENTITY_TYPE_SHIFT);            
    fprintf(stdout, "Entity Instance: 0x%x\n",
            pdr->entity_instance);
    fprintf(stdout, "Container ID: 0x%x\n",
            pdr->container_id);
    fprintf(stdout, "Sensor Init: %d\n",
            pdr->sensor_init);
    fprintf(stdout, "Sensor Auxiliary Names PDR: %d\n",
            pdr->sensor_auxiliary_names_pdr);
    fprintf(stdout, "Composite Sensor Count: %d\n",
            pdr->composite_sensor_count);
    struct state_sensor_possible_states *possible_states = (struct state_sensor_possible_states *)pdr->possible_states;
    fprintf(stdout, "Possible States Count: %d\n",
            possible_states->possible_states_size);
    fprintf(stdout, "State Set ID: 0x%x\n",
            possible_states->state_set_id);
    fprintf(stdout, "Possible States Size: %d\n",
            possible_states->possible_states_size);
    fprintf(stdout, "Possible States Bitfield: ");
    for (int i = 0; i < possible_states->possible_states_size; i++)
    {
        fprintf(stdout, "%02x ", possible_states->states[i].byte);
    }
    fprintf(stdout, "\n");
}

int parse_state_sensor_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_STATE_SENSOR_PDR);
    platform_ctx->state_sensor_record_count = count;

    platform_ctx->state_sensor_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->state_sensor_pdr == NULL)
    {
        fprintf(stderr, "Failed to find State Sensor PDR\n");
        free(platform_ctx->state_sensor_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_STATE_SENSOR_PDR, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->state_sensor_pdr[count] = (struct pldm_state_sensor_pdr *)malloc(size);
        if (platform_ctx->state_sensor_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for State Sensor PDR\n");
            free(platform_ctx->state_sensor_pdr);
            platform_ctx->state_sensor_pdr = NULL;
            return -ENOMEM;
        }
        memcpy(platform_ctx->state_sensor_pdr[count], data, size);
        count++;
    }

    return PLDM_SUCCESS;
}
void sensor_auxiliary_names_pdr_print(struct pldm_sensor_auxiliary_names_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid Sensor Auxiliary Names PDR\n");
        return;
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "Sensor Auxiliary Names PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "Sensor Auxiliary Names Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->terminus_handle);
    fprintf(stdout, "Sensor ID: 0x%x\n",
            pdr->sensor_id);
    fprintf(stdout, "Sensor Count: %d\n",
            pdr->sensor_count);
    fprintf(stdout, "Name String Count: %d\n",
            pdr->name_string_count);
    fprintf(stdout, "Sensor Auxiliary Names: ");
    int str_size = pdr->hdr.length - 6;
    for (int i = 0; i < str_size; i++)
    {
        uint16_t ch = pdr->sensor_auxiliary_names[i];
        if (ch < 0x20 || ch > 0x7E) // Check for printable ASCII characters
        {
            //fprintf(stderr, "Invalid character in Sensor Auxiliary Names PDR: 0x%x\n", ch);
            continue; // Skip invalid characters
        }
        fprintf(stdout, "%c", ch);
    }
    fprintf(stdout, "\n");
}

int parse_sensor_auxiliary_names_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_SENSOR_AUXILIARY_NAMES_PDR);
    platform_ctx->sensor_auxiliary_names_record_count = count;

    platform_ctx->sensor_auxiliary_names_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->sensor_auxiliary_names_pdr == NULL)
    {
        fprintf(stderr, "Failed to find Sensor Auxiliary Names PDR\n");
        free(platform_ctx->sensor_auxiliary_names_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_SENSOR_AUXILIARY_NAMES_PDR, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->sensor_auxiliary_names_pdr[count] = (struct pldm_sensor_auxiliary_names_pdr *)malloc(size);
        if (platform_ctx->sensor_auxiliary_names_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for Sensor Auxiliary Names PDR\n");
            free(platform_ctx->sensor_auxiliary_names_pdr);
            platform_ctx->sensor_auxiliary_names_pdr = NULL;
            return -ENOMEM;
        }
        memcpy(platform_ctx->sensor_auxiliary_names_pdr[count], data, size);
        count++;
    }
    return PLDM_SUCCESS; // Success
}

void parse_numeric_effecter_pdr_print(struct pldm_numeric_effecter_value_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid Numeric Effecter PDR\n");
        return;
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "Numeric Effecter PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "Numeric Effecter Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->terminus_handle);
    fprintf(stdout, "Effecter ID: 0x%x\n",
            pdr->effecter_id);
    fprintf(stdout, "Entity Type: 0x%x\n",
            pdr->entity_type & ENTITY_TYPE_MASK);
    fprintf(stdout, "Physical Entity : %d\n", (pdr->entity_type & ENTITY_TYPE_MASK) >> ENTITY_TYPE_SHIFT);            
    fprintf(stdout, "Entity Instance: 0x%x\n",
            pdr->entity_instance);
    fprintf(stdout, "Container ID: 0x%x\n",
            pdr->container_id);
    fprintf(stdout, "Effecter Init: %d\n",
            pdr->effecter_init);
    fprintf(stdout, "Effecter Auxiliary Names PDR: %d\n",
            pdr->effecter_auxiliary_names);
    fprintf(stdout, "Base Unit: %d\n",
            pdr->base_unit);
    fprintf(stdout, "Unit Modifier: %d\n",
            pdr->unit_modifier);
    fprintf(stdout, "Rate Unit: %d\n",
            pdr->rate_unit);
    fprintf(stdout, "Base OEM Unit Handle: 0x%x\n",
            pdr->base_oem_unit_handle);
    fprintf(stdout, "Auxiliary Unit: %d\n",
            pdr->aux_unit);
    fprintf(stdout, "Auxiliary Unit Modifier: %d\n",
            pdr->aux_unit_modifier);
    fprintf(stdout, "Auxiliary Rate Unit: %d\n",
            pdr->aux_rate_unit);
    fprintf(stdout, "Auxiliary OEM Unit Handle: 0x%x\n",
            pdr->aux_oem_unit_handle);
    fprintf(stdout, "Is Linear: %d\n",
            pdr->is_linear);
    fprintf(stdout, "Effecter Data Size: %d\n",
            pdr->effecter_data_size);
    fprintf(stdout, "Resolution: %f\n",
            pdr->resolution);
    fprintf(stdout, "Offset: %f\n",
            pdr->offset);
    fprintf(stdout, "Accuracy: %d\n",
            pdr->accuracy);
    fprintf(stdout, "Plus Tolerance: %d\n",
            pdr->plus_tolerance);
    fprintf(stdout, "Minus Tolerance: %d\n",
            pdr->minus_tolerance);
    fprintf(stdout, "State Transition Interval: %f\n",
            pdr->state_transition_interval);
    fprintf(stdout, "Transition Interval: %f\n",
            pdr->transition_interval);
    fprintf(stdout, "Max Set Table: %d\n",
            pdr->max_set_table.value_s32);
    fprintf(stdout, "Min Set Table: %d\n",
            pdr->min_set_table.value_u32);
    fprintf(stdout, "Range Field Format: %d\n",
            pdr->range_field_format);
    fprintf(stdout, "Range Field Support: %d\n",
            pdr->range_field_support.byte);
    fprintf(stdout, "Nominal Value: %d\n",
            pdr->nominal_value.value_u32);
    fprintf(stdout, "Normal Max: %d\n",
            pdr->normal_max.value_u32);
    fprintf(stdout, "Normal Min: %d\n",
            pdr->normal_min.value_u32);
    fprintf(stdout, "Rated Max: %d\n",
            pdr->rated_max.value_u32);
    fprintf(stdout, "Rated Min: %d\n",
            pdr->rated_min.value_u32);

    fprintf(stdout, "\n");
}

/* @brief Parses Numeric Effecter PDR from the PDR Repo from PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int parse_numeric_effecter_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_NUMERIC_EFFECTER_PDR);
    platform_ctx->numeric_effecter_record_count = count;

    platform_ctx->numeric_effecter_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->numeric_effecter_pdr == NULL)
    {
        fprintf(stderr, "Failed to find Numeric Effecter PDR\n");
        free(platform_ctx->numeric_effecter_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_NUMERIC_EFFECTER_PDR, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->numeric_effecter_pdr[count] = (struct pldm_numeric_effecter_value_pdr *)malloc(size);
        if (platform_ctx->numeric_effecter_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for Numeric Effecter PDR\n");
            free(platform_ctx->numeric_effecter_pdr);
            platform_ctx->numeric_effecter_pdr = NULL;
            return -ENOMEM;
        }
        // memcpy(platform_ctx->numeric_effecter_pdr[count], data, size);
        pldm_numeric_effecter_pdr_parse((uint8_t *)data, size, (uint8_t *)platform_ctx->numeric_effecter_pdr[count]);
        count++;
    }
    return PLDM_SUCCESS; // Success
}

void state_effecter_pdr_print(struct pldm_state_effecter_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid State Effecter PDR\n");
        return;
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "State Effecter PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "State Effecter Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->terminus_handle);
    fprintf(stdout, "Effecter ID: 0x%x\n",
            pdr->effecter_id);
    fprintf(stdout, "Entity Type: 0x%x\n",
            pdr->entity_type & ~ENTITY_TYPE_MASK);
    fprintf(stdout, "Physical Entity : %d\n", (pdr->entity_type & ENTITY_TYPE_MASK) >> ENTITY_TYPE_SHIFT);            
    fprintf(stdout, "Entity Instance: 0x%x\n",
            pdr->entity_instance);
    fprintf(stdout, "Container ID: 0x%x\n",
            pdr->container_id);
    fprintf(stdout, "Effecter Semantic ID: 0x%x\n",
            pdr->effecter_semantic_id);
    fprintf(stdout, "Effecter Init: %d\n",
            pdr->effecter_init);
    fprintf(stdout, "Has Description PDR: %d\n",
            pdr->has_description_pdr);
    fprintf(stdout, "Composite Effecter Count: %d\n",
            pdr->composite_effecter_count);
    fprintf(stdout, "Possible States: ");
    for (int i = 0; i < pdr->hdr.length - 15; i++)
    {
        fprintf(stdout, "%02x ", pdr->possible_states[i]);
    }
    fprintf(stdout, "\n");
}

/* @brief Parses State Effecter PDR from the PDR Repo from PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int parse_state_effecter_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_STATE_EFFECTER_PDR);
    platform_ctx->state_effecter_record_count = count;

    platform_ctx->state_effecter_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->state_effecter_pdr == NULL)
    {
        fprintf(stderr, "Failed to find State Effecter PDR\n");
        free(platform_ctx->state_effecter_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_STATE_EFFECTER_PDR, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->state_effecter_pdr[count] = (struct pldm_state_effecter_pdr *)malloc(size);
        if (platform_ctx->state_effecter_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for State Effecter PDR\n");
            free(platform_ctx->state_effecter_pdr);
            platform_ctx->state_effecter_pdr = NULL;
            return -ENOMEM;
        }
        memcpy(platform_ctx->state_effecter_pdr[count], data, size);
        count++;
    }

    return PLDM_SUCCESS; // Success
}

void entity_association_pdr_print(struct pldm_pdr_entity_association *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid Entity Association PDR\n");
        return;
    }
    struct pldm_pdr_entity_association {
	uint16_t container_id;
	uint8_t association_type;
	pldm_entity container;
	uint8_t num_children;
	pldm_entity children[1];
    };

    fprintf(stdout, "\n");
    fprintf(stdout, "Container ID: 0x%x\n",
            pdr->container_id);
    fprintf(stdout, "Association Type: %d\n",
            pdr->association_type);
    fprintf(stdout, "Container Entity Type: 0x%x\n",
            pdr->container.entity_type & ~ENTITY_TYPE_MASK);
    fprintf(stdout, "Physical Entity : %d\n", (pdr->container.entity_type & ENTITY_TYPE_MASK) >> ENTITY_TYPE_SHIFT);            
    fprintf(stdout, "Container Entity Instance Number: 0x%x\n",
            pdr->container.entity_instance_num);
    fprintf(stdout, "Entity Container ID: 0x%x\n",
            pdr->container.entity_container_id );
    fprintf(stdout, "Number of Children: %d\n",
            pdr->num_children);
    fprintf(stdout, "Children Entities:\n");
    for (int i = 0; i < pdr->num_children; i++)
    {
        fprintf(stdout, "  Child %d: Entity Type: 0x%x, Physical: %d, Entity Instance Number: 0x%x, Entity Container ID: 0x%x\n",
                i + 1,
                pdr->children[i].entity_type & ~ENTITY_TYPE_MASK,
                (pdr->children[i].entity_type & ENTITY_TYPE_MASK) >> ENTITY_TYPE_SHIFT,
                pdr->children[i].entity_instance_num,
                pdr->children[i].entity_container_id);
    }
    fprintf(stdout, "\n");
}   

/** @brief Prints the Entity Association PDR record.
 * @param pdr Pointer to the Entity Association PDR record
 * @return 0 on success, negative error code on failure
 */
int parse_entity_association_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_PDR_ENTITY_ASSOCIATION);
    platform_ctx->entity_association_record_count = count;

    platform_ctx->entity_association_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->entity_association_pdr == NULL)
    {
        fprintf(stderr, "Failed to find Entity Association PDR\n");
        free(platform_ctx->entity_association_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_PDR_ENTITY_ASSOCIATION, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->entity_association_pdr[count] = (struct pldm_pdr_entity_association *)malloc(size);
        if (platform_ctx->entity_association_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for Entity Association PDR\n");
            free(platform_ctx->entity_association_pdr);
            platform_ctx->entity_association_pdr = NULL;
            return -ENOMEM;
        }
        memcpy(platform_ctx->entity_association_pdr[count], data, size);
        count++;
    }
    return PLDM_SUCCESS; // Success
}

void effecter_auxiliary_names_pdr_print(struct pldm_effecter_auxiliary_names_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid Effecter Auxiliary Names PDR\n");
        return;
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "Effecter Auxiliary Names PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "Effecter Auxiliary Names Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->terminus_handle);
    fprintf(stdout, "Effecter ID: 0x%x\n",
            pdr->effecter_id);
    fprintf(stdout, "Effecter Count: %d\n",
            pdr->effecter_count);
    fprintf(stdout, "Name String Count: %d\n",
            pdr->name_string_count);
    fprintf(stdout, "Effecter Auxiliary Names: ");
    int str_size = pdr->hdr.length - 6;
    for (int i = 0; i < str_size; i++)
    {
        uint16_t ch = pdr->effecter_auxiliary_names[i];
        if (ch < 0x20 || ch > 0x7E) // Check for printable ASCII characters
        {
            //fprintf(stderr, "Invalid character in Effecter Auxiliary Names PDR: 0x%x\n", ch); 
            continue; // Skip invalid characters
        }
        fprintf(stdout, "%c", ch);
    }
    fprintf(stdout, "\n");
}

/* @brief Parses Effecter Auxiliary Names PDR from the PDR Repo from PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int parse_effecter_auxiliary_names_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_EFFECTER_AUXILIARY_NAMES_PDR);
    platform_ctx->effecter_auxiliary_names_record_count = count;

    platform_ctx->effecter_auxiliary_names_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->effecter_auxiliary_names_pdr == NULL)
    {
        fprintf(stderr, "Failed to find Effecter Auxiliary Names PDR\n");
        free(platform_ctx->effecter_auxiliary_names_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_EFFECTER_AUXILIARY_NAMES_PDR, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        platform_ctx->effecter_auxiliary_names_pdr[count] = (struct pldm_effecter_auxiliary_names_pdr *)malloc(size);
        if (platform_ctx->effecter_auxiliary_names_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for Effecter Auxiliary Names PDR\n");
            free(platform_ctx->effecter_auxiliary_names_pdr);
            platform_ctx->effecter_auxiliary_names_pdr = NULL;
            return -ENOMEM;
        }
        memcpy(platform_ctx->effecter_auxiliary_names_pdr[count], data, size);
        count++;
    }

    return PLDM_SUCCESS; // Success
}

void pldm_fru_record_set_pdr_print(struct pldm_fru_record_set_pdr *pdr)
{
    if (pdr == NULL)
    {
        fprintf(stderr, "Invalid FRU Record Set PDR\n");
        return;
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "FRU Record Set PDR Record Handle: %u, Size: %u\n",
            pdr->hdr.record_handle,
            pdr->hdr.length);
    fprintf(stdout, "FRU Record Set Type: %s\n",
            pdr_type_to_string(pdr->hdr.type));
    fprintf(stdout, "Terminus Handle: 0x%x\n",
            pdr->fru_record_set.terminus_handle);
    fprintf(stdout, "FRU Record Set RSI: 0x%x\n",
            pdr->fru_record_set.fru_rsi);
    fprintf(stdout, "Entity Type: 0x%x\n",
            pdr->fru_record_set.entity_type & ~ENTITY_TYPE_MASK);
    fprintf(stdout, "Physical Entity : %d\n", (pdr->fru_record_set.entity_type & ENTITY_TYPE_MASK) >> ENTITY_TYPE_SHIFT);
    fprintf(stdout, "Entity Instance: 0x%x\n",
            pdr->fru_record_set.entity_instance_num);
    fprintf(stdout, "Container ID: 0x%x\n",
            pdr->fru_record_set.container_id);
}

/* @brief Parses FRU Record Set PDR from the PDR Repo from PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int parse_fru_record_set_pdr(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;
    int count = get_num_pdrs_by_type(ctx, PLDM_PDR_FRU_RECORD_SET);
    platform_ctx->fru_record_set_record_count = count;

    platform_ctx->fru_record_set_pdr = malloc(sizeof(void *) * count);
    if (platform_ctx->fru_record_set_pdr == NULL)
    {
        fprintf(stderr, "Failed to find FRU Record Set PDR\n");
        free(platform_ctx->fru_record_set_pdr);
        return -ENOENT;
    }
    const struct pldm_pdr_record *record = NULL;
    int record_count = platform_ctx->pdr_repository_info.record_count; // Count the first record
    uint8_t *data = NULL;
    uint32_t size = 0;
    count = 0;
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(platform_ctx->pdrs, PLDM_PDR_FRU_RECORD_SET, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more FRU Record Set PDR records found\n");
            break;
        }
        platform_ctx->fru_record_set_pdr[count] = (struct pldm_fru_record_set_pdr *)malloc(size);
        if (platform_ctx->fru_record_set_pdr[count] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for FRU Record Set PDR\n");
            free(platform_ctx->fru_record_set_pdr);
            platform_ctx->fru_record_set_pdr = NULL;
            return -ENOMEM;
        }
        memcpy(platform_ctx->fru_record_set_pdr[count], data, size);
        count++;
    }
    //pldm_fru_record_set_pdr_print(platform_ctx->fru_record_set_pdr[0]);
    return PLDM_SUCCESS; // Success
}

/*@brief Prints the PDR records in the PLDM context.
 * @param ctx Pointer to the PLDM context.
 */
void pdr_print(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return;
    }

    uint32_t record_handle = 1; // Start with the first record handle
    uint8_t *data = malloc(ctx->platform_ctx->pdr_repository_info.largest_record_size);
    if (data == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for PDR record data\n");
        return;
    }
    memset(data, 0, ctx->platform_ctx->pdr_repository_info.largest_record_size);
    uint32_t size;
    uint32_t next_record_handle = -1; // Initialize to -1 to start with the first record

    const struct pldm_pdr_record *record = pldm_pdr_find_record(ctx->platform_ctx->pdrs,
                                                                record_handle,
                                                                &data, &size,
                                                                &next_record_handle);
    if (record == NULL)
    {
        fprintf(stderr, "No PDR records found\n");
        free(data);
        return;
    }
    printf("PDR Record Handle: %u, Size: %u, Next Record Handle: %d\n", record_handle, size, next_record_handle);

    int record_count = 10; // Count the first record
    while (record_count-- > 0 && record != NULL)
    {
        record_handle = next_record_handle;
        record = pldm_pdr_get_next_record(ctx->platform_ctx->pdrs, record, &data, &size, &next_record_handle);
        if (record == NULL)
        {
            fprintf(stderr, "No more PDR records found\n");
            break;
        }
        printf("PDR Record Handle: %u, Size: %u, Next Record Handle: %d\n", record_handle, size, next_record_handle);
    }
}
/*
#define SET_STATE_SENSOR_ENABLES     0x20
#define GET_STATE_SENSOR_READINGS    0x21
#define SET_STATE_EFFECTER_ENABLE    0x38
#define SET_STATE_EFFECTER_STATE     0x39
#define GET_STATE_EFFECTER_STATES    0x3A
*/

/* @brief Prints the numeric sensor PDR records in the PLDM context.
 * @param ctx Pointer to the PLDM context.
 */
void pdr_print_by_type(pldm_ctx_t *ctx, uint8_t pdr_type)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return;
    }

    uint8_t *data = NULL;
    uint32_t size;

    const struct pldm_pdr_record *record = NULL;

    int record_count = ctx->platform_ctx->pdr_repository_info.record_count; // Count the first record
    while (record_count-- > 0)
    {
        record = pldm_pdr_find_record_by_type(ctx->platform_ctx->pdrs, pdr_type, record, &data, &size);
        if (record == NULL)
        {
            // fprintf(stderr, "No more sensor PDR records found\n");
            break;
        }
        uint32_t record_handle = pldm_pdr_get_record_handle(ctx->platform_ctx->pdrs, record);
        printf("Sensor Type: %s PDR Record Handle: %u, Size: %u\n", pdr_type_to_string(pdr_type), record_handle, size);
    }
}

void *pdr_alloc(uint8_t pdr_type, uint8_t count)
{
    if (count == 0)
    {
        fprintf(stderr, "Invalid count for PDR allocation\n");
        return NULL;
    }

    switch (pdr_type)
    {
    case PLDM_TERMINUS_LOCATOR_PDR:
        return malloc(sizeof(struct pldm_terminus_locator_pdr) * count);

    case PLDM_NUMERIC_SENSOR_PDR:
        return malloc(sizeof(struct pldm_numeric_sensor_value_pdr) * count);

    case PLDM_STATE_SENSOR_PDR:
        return malloc(sizeof(struct pldm_state_sensor_pdr) * count);

    case PLDM_SENSOR_AUXILIARY_NAMES_PDR:
        return malloc(sizeof(struct pldm_sensor_auxiliary_names_pdr) * count);

    case PLDM_NUMERIC_EFFECTER_PDR:
        return malloc(sizeof(struct pldm_numeric_effecter_value_pdr) * count);

    case PLDM_STATE_EFFECTER_PDR:
        return malloc(sizeof(struct pldm_state_effecter_pdr) * count);

    case PLDM_EFFECTER_AUXILIARY_NAMES_PDR:
        return malloc(sizeof(struct pldm_effecter_auxiliary_names_pdr) * count);
    case PLDM_PDR_ENTITY_ASSOCIATION:
        return malloc(sizeof(struct pldm_pdr_entity_association) * count);

    case PLDM_ENTITY_AUXILIARY_NAMES_PDR:
        return malloc(sizeof(struct pldm_sensor_auxiliary_names_pdr) * count);

    case PLDM_PDR_FRU_RECORD_SET:
        return malloc(sizeof(struct pldm_pdr_fru_record_set) * count);

    default:
        fprintf(stderr, "Unknown PDR type: %d\n", pdr_type);
        return NULL;
    }

    return NULL;
}
/*
 * @brief Parses all PDR records in the PLDM context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int parse_all_pdrs(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }

    int rc;
    rc = parse_terminus_locator_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse Terminus Locator PDR: %d\n", rc);
        return rc;
    }

    rc = parse_numeric_sensor_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse Numeric Sensor PDR: %d\n", rc);
        return rc;
    }

    rc = parse_numeric_effecter_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse Numeric Effecter PDR: %d\n", rc);
        return rc;
    }

    rc = parse_state_sensor_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse State Sensor PDR: %d\n", rc); 
        return rc;
    }

    rc = parse_sensor_auxiliary_names_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse Sensor Auxiliary Names PDR: %d\n", rc);
        return rc;
    }

    rc = parse_state_effecter_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse State Effecter PDR: %d\n", rc);
        return rc;
    }

    rc = parse_effecter_auxiliary_names_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse Effecter Auxiliary Names PDR: %d\n", rc);
        return rc;
    }

    rc = parse_fru_record_set_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse FRU Record Set PDR: %d\n", rc);
        return rc;
    }

    rc = parse_entity_association_pdr(ctx);
    if (rc && rc != -ENOENT)
    {
        fprintf(stderr, "Failed to parse Entity Association PDR: %d\n", rc);
        return rc;
    }
    return 0;
}

int aux_names_print(uint8_t *tag, uint8_t tag_length, 
                        uint16_t *aux_name, uint8_t aux_length)
{
    if (tag == NULL || tag_length == 0 || aux_name == NULL || aux_length == 0)
    {
        fprintf(stderr, "Invalid parameters for printing auxiliary names\n");
        return -EINVAL;
    }

    fprintf(stdout, "Tag: %.*s ", tag_length, tag);
    fprintf(stdout, "Name: ");
    
    for (int i = 0; i < aux_length/2; i++)
    {
        uint16_t ch = ((aux_name[i] & 0x00FF) << 8) | ((aux_name[i] & 0xFF00) >> 8);
        fprintf(stdout, "%c", ch);
    }
    fprintf(stdout, "\n");

    return PLDM_SUCCESS;
}

int parse_aux_names(uint8_t *aux_name, uint8_t aux_length, 
                        uint8_t *tag_name, uint8_t *tag_length, 
                        uint16_t *name, uint8_t *name_length)
{
    if (aux_name == NULL || aux_length == 0 || tag_name == NULL || tag_length == NULL || name == NULL || name_length == NULL)
    {
        fprintf(stderr, "Invalid parameters for parsing auxiliary names\n");
        return -EINVAL;
    }

    if (*tag_length < 3 || aux_length < 3)
    {
        fprintf(stderr, "Invalid tag length or auxiliary name length\n");
        return -EINVAL;
    }

    // Print the auxiliary name
    int len = snprintf((char *)tag_name, (size_t)tag_length, "%s", aux_name);
    // fprintf(stdout, "Tag: %s ", tag_name);

    if (len < 0 || len >= *tag_length)
    {
        fprintf(stderr, "Failed to copy auxiliary name to tag_name\n");
        return -EINVAL;
    }

    if (aux_length <= len + 1)
    {
        fprintf(stderr, "Auxiliary name length is too short for name extraction\n");
        return -EINVAL;
    }

    if (aux_length - len - 1 <= 0)
    {
        fprintf(stderr, "No name data available after tag name\n");
        return -EINVAL;
    }
    
    *name_length = aux_length - len - 1;
    memcpy(name, aux_name + len + 1, *name_length);

    return PLDM_SUCCESS;
}

int id2aux_names_print(struct id2name_map *id2name_map, int count)
{
    if (id2name_map == NULL || count <= 0)
    {
        fprintf(stderr, "Invalid ID to Auxiliary Names Map\n");
        return -EINVAL;
    }

    for (int i = 0; i < count; i++)
    {
        fprintf(stdout, "ID: 0x%x ", id2name_map[i].id);
        aux_names_print(id2name_map[i].tag, id2name_map[i].tag_length,
                        id2name_map[i].auxiliary_name, id2name_map[i].auxiliary_name_length);
    }
    return PLDM_SUCCESS;
}

int pldm_numeric_sensor_pdr_map_id2name(pldm_platform_ctx_t *pctx)
{
    if (pctx == NULL)
    {
        fprintf(stderr, "Invalid Numeric Sensor PDR\n");
        return -EINVAL;
    }
  
    pctx->numeric_sensor_id2name_map = malloc(sizeof(struct id2name_map) * pctx->numeric_sensor_record_count);
    if (pctx->numeric_sensor_id2name_map == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for Numeric Sensor ID to Name Map\n");
        return -ENOMEM;
    }
    memset(pctx->numeric_sensor_id2name_map, 0, sizeof(struct id2name_map) * pctx->numeric_sensor_record_count);

    struct id2name_map *id2name_map = pctx->numeric_sensor_id2name_map;
    pctx->numeric_sensor_id2name_map_count = 0;

    for (int i = 0; i < pctx->numeric_sensor_record_count; i++)
    {
        struct pldm_numeric_sensor_value_pdr *pdr = pctx->numeric_sensor_pdr[i];

        if (pdr == NULL)
        {
            fprintf(stderr, "Invalid Numeric Sensor PDR at index %d\n", i);
            continue;
        }

        uint8_t sensor_id = pdr->sensor_id;
        if (sensor_id == 0)
        {
            fprintf(stderr, "Invalid Sensor ID: 0x%x\n", sensor_id);
            continue;
        }

        if (pdr->sensor_auxiliary_names_pdr) {
            for (int j = 0; j < pctx->sensor_auxiliary_names_record_count; j++)
            {
                struct pldm_sensor_auxiliary_names_pdr *aux_pdr = pctx->sensor_auxiliary_names_pdr[j];
                if (aux_pdr == NULL)
                {
                    fprintf(stderr, "Invalid Sensor Auxiliary Names PDR at index %d\n", j);
                    continue;
                }
                if (pdr->sensor_id == aux_pdr->sensor_id)
                {
                    id2name_map[pctx->numeric_sensor_id2name_map_count].id = sensor_id;

                    id2name_map[pctx->numeric_sensor_id2name_map_count].tag_length = 32;
                    id2name_map[pctx->numeric_sensor_id2name_map_count].auxiliary_name_length = 255;

                    parse_aux_names(aux_pdr->sensor_auxiliary_names, aux_pdr->hdr.length - 6,
                        id2name_map[pctx->numeric_sensor_id2name_map_count].tag,
                        &id2name_map[pctx->numeric_sensor_id2name_map_count].tag_length,
                        id2name_map[pctx->numeric_sensor_id2name_map_count].auxiliary_name,
                        &id2name_map[pctx->numeric_sensor_id2name_map_count].auxiliary_name_length);

                    pctx->numeric_sensor_id2name_map_count++;
                    break;
                }
            }
        }
    }  
    return PLDM_SUCCESS;
}

int pldm_numeric_effecter_pdr_map_id2name(pldm_platform_ctx_t *pctx)
{
    if (pctx == NULL)
    {
        fprintf(stderr, "Invalid Numeric Effecter PDR\n");
        return -EINVAL;
    }

    pctx->numeric_effecter_id2name_map = malloc(sizeof(struct id2name_map) * pctx->numeric_effecter_record_count);
    if (pctx->numeric_effecter_id2name_map == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for Numeric Effecter ID to Name Map\n");
        return -ENOMEM;
    }
    memset(pctx->numeric_effecter_id2name_map, 0, sizeof(struct id2name_map) * pctx->numeric_effecter_record_count);

    struct id2name_map *id2name_map = pctx->numeric_effecter_id2name_map;
    pctx->numeric_effecter_id2name_map_count = 0;

    for (int i = 0; i < pctx->numeric_effecter_record_count; i++)
    {
        struct pldm_numeric_effecter_value_pdr *pdr = pctx->numeric_effecter_pdr[i];

        if (pdr == NULL)
        {
            fprintf(stderr, "Invalid Numeric Effecter PDR at index %d\n", i);
            continue;
        }

        uint8_t effecter_id = pdr->effecter_id;
        if (effecter_id == 0)
        {
            fprintf(stderr, "Invalid Effecter ID: 0x%x\n", effecter_id);
            continue;
        }

        if (pdr->effecter_auxiliary_names) {
            for (int j = 0; j < pctx->effecter_auxiliary_names_record_count; j++)
            {
                struct pldm_effecter_auxiliary_names_pdr *aux_pdr = pctx->effecter_auxiliary_names_pdr[j];
                if (aux_pdr == NULL)
                {
                    fprintf(stderr, "Invalid Effecter Auxiliary Names PDR at index %d\n", j);
                    continue;
                }
                if (pdr->effecter_id == aux_pdr->effecter_id)
                {
                    id2name_map[pctx->numeric_effecter_id2name_map_count].id = effecter_id;

                    id2name_map[pctx->numeric_effecter_id2name_map_count].tag_length = 32;
                    id2name_map[pctx->numeric_effecter_id2name_map_count].auxiliary_name_length = 255;
                    parse_aux_names(aux_pdr->effecter_auxiliary_names, aux_pdr->hdr.length - 6,
                        id2name_map[pctx->numeric_effecter_id2name_map_count].tag,
                        &id2name_map[pctx->numeric_effecter_id2name_map_count].tag_length,
                        id2name_map[pctx->numeric_effecter_id2name_map_count].auxiliary_name,
                        &id2name_map[pctx->numeric_effecter_id2name_map_count].auxiliary_name_length);
                    pctx->numeric_effecter_id2name_map_count++;
                    break;
                }
            }
        }
    }

    fprintf(stdout, "Numeric Effecter ID to Name Map Count: %d\n", pctx->numeric_effecter_id2name_map_count);
    if (pctx->numeric_effecter_id2name_map_count == 0)
    {
        fprintf(stderr, "No Numeric Effecter ID to Name Map found\n");
        free(pctx->numeric_effecter_id2name_map);
        pctx->numeric_effecter_id2name_map = NULL;
        return -ENOENT;
    }
    return PLDM_SUCCESS;
}

int pldm_state_sensor_pdr_map_id2name(pldm_platform_ctx_t *pctx)
{
    if (pctx == NULL)
    {
        fprintf(stderr, "Invalid State Sensor PDR\n");
        return -EINVAL;
    }

    pctx->state_sensor_id2name_map = malloc(sizeof(struct id2name_map) * pctx->state_sensor_record_count);
    if (pctx->state_sensor_id2name_map == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for State Sensor ID to Name Map\n");
        return -ENOMEM;
    }
    memset(pctx->state_sensor_id2name_map, 0, sizeof(struct id2name_map) * pctx->state_sensor_record_count);

    struct id2name_map *id2name_map = pctx->state_sensor_id2name_map;
    pctx->state_sensor_id2name_map_count = 0;

    for (int i = 0; i < pctx->state_sensor_record_count; i++)
    {
        struct pldm_state_sensor_pdr *pdr = pctx->state_sensor_pdr[i];

        if (pdr == NULL)
        {
            fprintf(stderr, "Invalid State Sensor PDR at index %d\n", i);
            continue;
        }

        uint8_t sensor_id = pdr->sensor_id;
        if (sensor_id == 0)
        {
            fprintf(stderr, "Invalid Sensor ID: 0x%x\n", sensor_id);
            continue;
        }

        if (pdr->sensor_auxiliary_names_pdr) {
            for (int j = 0; j < pctx->sensor_auxiliary_names_record_count; j++)
            {
                struct pldm_sensor_auxiliary_names_pdr *aux_pdr = pctx->sensor_auxiliary_names_pdr[j];
                if (aux_pdr == NULL)
                {
                    fprintf(stderr, "Invalid Sensor Auxiliary Names PDR at index %d\n", j);
                    continue;
                }
                if (pdr->sensor_id == aux_pdr->sensor_id)
                {
                    id2name_map[pctx->state_sensor_id2name_map_count].id = sensor_id;

                    id2name_map[pctx->state_sensor_id2name_map_count].tag_length = 32;
                    id2name_map[pctx->state_sensor_id2name_map_count].auxiliary_name_length = 255;

                    parse_aux_names(aux_pdr->sensor_auxiliary_names, aux_pdr->hdr.length - 6,
                        id2name_map[pctx->state_sensor_id2name_map_count].tag,
                        &id2name_map[pctx->state_sensor_id2name_map_count].tag_length,
                        id2name_map[pctx->state_sensor_id2name_map_count].auxiliary_name,
                        &id2name_map[pctx->state_sensor_id2name_map_count].auxiliary_name_length);
                    pctx->state_sensor_id2name_map_count++;
                    break;
                }
            }
        }
    }
    fprintf(stdout, "State Sensor ID to Name Map Count: %d\n", pctx->state_sensor_id2name_map_count);
    if (pctx->state_sensor_id2name_map_count == 0)
    {
        fprintf(stderr, "No State Sensor ID to Name Map found\n");
        free(pctx->state_sensor_id2name_map);
        pctx->state_sensor_id2name_map = NULL;
        return -ENOENT;
    }
    return PLDM_SUCCESS;
}

int pldm_state_effecter_pdr_map_id2name(pldm_platform_ctx_t *pctx)
{
    if (pctx == NULL)
    {
        fprintf(stderr, "Invalid State Effecter PDR\n");
        return -EINVAL;
    }

    pctx->state_effecter_id2name_map = malloc(sizeof(struct id2name_map) * pctx->state_effecter_record_count);
    if (pctx->state_effecter_id2name_map == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for State Effecter ID to Name Map\n");
        return -ENOMEM;
    }
    memset(pctx->state_effecter_id2name_map, 0, sizeof(struct id2name_map) * pctx->state_effecter_record_count);

    struct id2name_map *id2name_map = pctx->state_effecter_id2name_map;
    pctx->state_effecter_id2name_map_count = 0;

    for (int i = 0; i < pctx->state_effecter_record_count; i++)
    {
        struct pldm_state_effecter_pdr *pdr = pctx->state_effecter_pdr[i];

        if (pdr == NULL)
        {
            fprintf(stderr, "Invalid State Effecter PDR at index %d\n", i);
            continue;
        }

        uint8_t effecter_id = pdr->effecter_id;
        if (effecter_id == 0)
        {
            fprintf(stderr, "Invalid Effecter ID: 0x%x\n", effecter_id);
            continue;
        }

        if (pdr->has_description_pdr) {
            for (int j = 0; j < pctx->effecter_auxiliary_names_record_count; j++)
            {
                struct pldm_effecter_auxiliary_names_pdr *aux_pdr = pctx->effecter_auxiliary_names_pdr[j];
                if (aux_pdr == NULL)
                {
                    fprintf(stderr, "Invalid Effecter Auxiliary Names PDR at index %d\n", j);
                    continue;
                }
                if (pdr->effecter_id == aux_pdr->effecter_id)
                {
                    id2name_map[pctx->state_effecter_id2name_map_count].id = effecter_id;

                    id2name_map[pctx->state_effecter_id2name_map_count].tag_length = 32;
                    id2name_map[pctx->state_effecter_id2name_map_count].auxiliary_name_length = 255;
                    parse_aux_names(aux_pdr->effecter_auxiliary_names, aux_pdr->hdr.length - 6,
                        id2name_map[pctx->state_effecter_id2name_map_count].tag,
                        &id2name_map[pctx->state_effecter_id2name_map_count].tag_length,
                        id2name_map[pctx->state_effecter_id2name_map_count].auxiliary_name,
                        &id2name_map[pctx->state_effecter_id2name_map_count].auxiliary_name_length);
                    pctx->state_effecter_id2name_map_count++;
                    break;
                }
            }
        }
    }
/*     fprintf(stdout, "State Effecter ID to Name Map Count: %d\n", pctx->state_effecter_id2name_map_count);
    if (pctx->state_effecter_id2name_map_count == 0)
    {
        fprintf(stderr, "No State Effecter ID to Name Map found\n");
        free(pctx->state_effecter_id2name_map);
        pctx->state_effecter_id2name_map = NULL;
        return -ENOENT;
    } */
    return PLDM_SUCCESS;
}

int map_id2name(pldm_ctx_t *ctx)
{
    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return -EINVAL;
    }

    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;

    /* pldm_numeric_sensor_pdr_map_id2name(platform_ctx);
    id2aux_names_print(platform_ctx->numeric_sensor_id2name_map, platform_ctx->numeric_sensor_id2name_map_count);
    pldm_numeric_effecter_pdr_map_id2name(platform_ctx);
    if (platform_ctx->numeric_effecter_id2name_map_count > 0)
    {
        id2aux_names_print(platform_ctx->numeric_effecter_id2name_map, platform_ctx->numeric_effecter_id2name_map_count);
    }
    pldm_state_sensor_pdr_map_id2name(platform_ctx);
    if (platform_ctx->state_sensor_id2name_map_count > 0)
    {
        id2aux_names_print(platform_ctx->state_sensor_id2name_map, platform_ctx->state_sensor_id2name_map_count);
    }
    */
    pldm_state_effecter_pdr_map_id2name(platform_ctx);
    if (platform_ctx->state_effecter_id2name_map_count > 0)
    {
        id2aux_names_print(platform_ctx->state_effecter_id2name_map, platform_ctx->state_effecter_id2name_map_count);
    }

    return PLDM_SUCCESS;
}

/**
 * @brief Dumps all PDR records in the PLDM Repo.
 * @param ctx Pointer to the PLDM context.
 */
void pldm_platform_info_print(pldm_ctx_t *ctx)
{
    fprintf(stdout, "\nPLDM Platform Information:\n");
    
    fprintf(stdout, "----------------------------\n");

    if (ctx == NULL || ctx->platform_ctx == NULL || ctx->platform_ctx->pdrs == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or PDR repository\n");
        return;
    }
    pldm_platform_ctx_t *platform_ctx = ctx->platform_ctx;

    buf_print("Terminus UID", platform_ctx->terminus_uid, UUID_SIZE);

    pdr_repository_info_print(&platform_ctx->pdr_repository_info);
    
    for (int count = 0; count < platform_ctx->terminus_locator_record_count; count++)
    {
        terminus_locator_pdr_print(platform_ctx->terminus_locator_pdr[count]);
    }

    for (int count = 0; count < platform_ctx->numeric_sensor_record_count; count++)
    {
        numeric_sensor_pdr_print(platform_ctx->numeric_sensor_pdr[count]);
    }

    for (int count = 0; count < platform_ctx->numeric_effecter_record_count; count++)
    {
        parse_numeric_effecter_pdr_print(platform_ctx->numeric_effecter_pdr[count]);
    }

    for (int count = 0; count < platform_ctx->state_sensor_record_count; count++)
    {   
        state_sensor_pdr_print(platform_ctx->state_sensor_pdr[count]);
    }

    for (int count = 0; count < platform_ctx->sensor_auxiliary_names_record_count; count++)
    {
        sensor_auxiliary_names_pdr_print(platform_ctx->sensor_auxiliary_names_pdr[count]); 
    }

    for (int count = 0; count < platform_ctx->state_effecter_record_count; count++)
    {
        state_effecter_pdr_print(platform_ctx->state_effecter_pdr[count]);
    }

    for (int count = 0; count < platform_ctx->effecter_auxiliary_names_record_count; count++)
    {
        effecter_auxiliary_names_pdr_print(platform_ctx->effecter_auxiliary_names_pdr[count]);
    }

    for (int count = 0; count < platform_ctx->fru_record_set_record_count; count++)
    {
        pldm_fru_record_set_pdr_print(platform_ctx->fru_record_set_pdr[count]);
    }

    for (int count = 0; count < platform_ctx->entity_association_record_count; count++)
   {
        entity_association_pdr_print(platform_ctx->entity_association_pdr[count]);
   }
}
/**
 * @brief Initializes the PLDM platform context.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int pldm_platform_init(pldm_ctx_t *ctx)
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

    // Initialize the PLDM platform context
    ctx->platform_ctx = malloc(sizeof(pldm_platform_ctx_t));
    if (ctx->platform_ctx == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for PLDM platform context\n");
        return -ENOMEM;
    }
    memset(ctx->platform_ctx, 0, sizeof(pldm_platform_ctx_t));

     int rc = get_pdr_repository_info(ctx);
    if (rc)
    {
        fprintf(stderr, "Failed to get PDR repository info: %d\n", rc);
        free(ctx->platform_ctx);
        ctx->platform_ctx = NULL;
        return rc;
    }

    rc = get_terminus_uid(ctx);
    if (rc)
    {
        fprintf(stderr, "Failed to get terminus UID: %d\n", rc);
        free(ctx->platform_ctx);
        ctx->platform_ctx = NULL;
        return rc;
    }

    rc = get_all_pdrs(ctx);
    if (rc)
    {
        fprintf(stderr, "Failed to get PDR: %d\n", rc);
        free(ctx->platform_ctx);
        ctx->platform_ctx = NULL;
        return rc;
    }
    rc = parse_all_pdrs(ctx);
    if (rc)
    {
        fprintf(stderr, "Failed to parse all PDRs: %d\n", rc);
        free(ctx->platform_ctx);
        ctx->platform_ctx = NULL;
        return rc;
    }

    rc = map_id2name(ctx);
    if (rc)
    {
        fprintf(stderr, "Failed to map ID to name: %d\n", rc);
        free(ctx->platform_ctx);
        ctx->platform_ctx = NULL;
        return rc;
    }

    //map_id2name_print(ctx);
    //fprintf(stdout, "PLDM platform context initialized successfully.\n");

    return 0; // Success
}
