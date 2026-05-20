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
#include "pldm.h"
#include "fwpkg.h"

extern pldm_ctx_t *pldm_ctx;

void pldm_fwu_print_error(pldm_ctx_t *ctx, const char *func_name, int rc)
{
    if (ctx == NULL || func_name == NULL)
    {
        fprintf(stderr, "Invalid context or function name\n");
        return;
    }

    enum fw_update_error_completion_codes completion_code = (enum fw_update_error_completion_codes)rc;

    if (rc  == PLDM_SUCCESS)
    {
        fprintf(stdout, "%s: Operation completed successfully\n", func_name);
        return;
    }

    switch (completion_code)
    {
    case NOT_IN_UPDATE_MODE:
        fprintf(stderr, "%s: Not in update mode\n", func_name);
        break;
    case ALREADY_IN_UPDATE_MODE:
        fprintf(stderr, "%s: Already in update mode\n", func_name);
        break;
    case DATA_OUT_OF_RANGE:
        fprintf(stderr, "%s: Data out of range\n", func_name);
        break;
    case INVALID_TRANSFER_LENGTH:
        fprintf(stderr, "%s: Invalid transfer length\n", func_name);
        break;
    case INVALID_STATE_FOR_COMMAND:
        fprintf(stderr, "%s: Invalid state for command\n", func_name);
        break;
    case INCOMPLETE_UPDATE:
        fprintf(stderr, "%s: Incomplete update\n", func_name);
        break;
    case BUSY_IN_BACKGROUND:
        fprintf(stderr, "%s: Busy in background operation\n", func_name);
        break;
    case CANCEL_PENDING:
        fprintf(stderr, "%s: Cancel pending\n", func_name);
        break;
    case COMMAND_NOT_EXPECTED:
        fprintf(stderr, "%s: Command not expected\n", func_name);
        break;
    case RETRY_REQUEST_FW_DATA:
        fprintf(stderr, "%s: Retry request firmware data\n", func_name);
        break;
    case UNABLE_TO_INITIATE_UPDATE:
        fprintf(stderr, "%s: Unable to initiate update\n", func_name);
        break;
    case ACTIVATION_NOT_REQUIRED:
        fprintf(stderr, "%s: Activation not required\n", func_name);
        break;
    case SELF_CONTAINED_ACTIVATION_NOT_PERMITTED:
        fprintf(stderr, "%s: Self-contained activation not permitted\n", func_name);
        break;
    case NO_DEVICE_METADATA:
        fprintf(stderr, "%s: No device metadata available\n", func_name);
        break;
    case RETRY_REQUEST_UPDATE:
        fprintf(stderr, "%s: Retry request update\n", func_name);
        break;
    case NO_PACKAGE_DATA:
        fprintf(stderr, "%s: No package data available\n", func_name);
        break;
    case INVALID_DATA_TRANSFER_HANDLE:
        fprintf(stderr, "%s: Invalid data transfer handle\n", func_name);
        break;
    case INVALID_TRANSFER_OPERATION_FLAG:
        fprintf(stderr, "%s: Invalid transfer operation flag\n", func_name);
        break;
    default:
        if (rc == PLDM_FWU_TIME_OUT) {
            fprintf(stderr, "%s: Timeout occurred\n", func_name);
            break;
        } else if (rc == PLDM_FWU_GENERIC_ERROR) {
            fprintf(stderr, "%s: Generic error occurred\n", func_name);
            break;
        }

        fprintf(stderr, "%s: Unknown error code %d\n", func_name, rc);
    }
    fprintf(stderr, "Error code: %d\n", rc);
}

int query_device_id(pldm_ctx_t *ctx, uint8_t **descriptor_data,
                    uint32_t *device_identifiers_len,
                    uint8_t *descriptor_count)
{
    fprintf(stdout, "%s entry\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t instanceId = get_instance_id();
    struct pldm_msg request;

    int rc = encode_query_device_identifiers_req(
        instanceId, &request, PLDM_QUERY_DEVICE_IDENTIFIERS_REQ_BYTES);

    if (rc)
    {
        fprintf(stderr, "Failed to encode query device identifiers request: %d\n", rc);
        return rc;
    }
    uint8_t response_buf[PLDM_MAX_CMDS_PER_TYPE];
    memset(response_buf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)response_buf;
    uint16_t response_len = 3 + PLDM_FWU_MIN_DESCRIPTOR_IDENTIFIERS_LEN + 15;

    uint16_t req_len = 3 + PLDM_QUERY_DEVICE_IDENTIFIERS_REQ_BYTES;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)&request, req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    response_len = 12;

    uint8_t completion_code;
    uint8_t *data;
    rc = decode_query_device_identifiers_resp(response, response_len,
                                              &completion_code,
                                              device_identifiers_len,
                                              descriptor_count,
                                              &data);
    if (rc)
    {
        fprintf(stderr, "Failed to decode query device identifiers response: %d\n", rc);
        return rc;
    }
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Query device identifiers failed with completion code: %d\n", completion_code);
        pldm_fwu_print_error(ctx, __func__, completion_code);
        return -1;
    }
    memcpy(*descriptor_data, data, *device_identifiers_len);

    return 0;
}

void dump_firmware_parameters(firmware_parameters_info_t *fw_params)
{
    fprintf(stdout, "Capabilities During Update: %d\n", fw_params->capabilities_during_update.value);
    fprintf(stdout, "Component Count: %d\n", fw_params->comp_count);
    fprintf(stdout, "Active Component Image Set Version String Type: %d\n",
            fw_params->active_comp_image_set_ver_str_type);
    fprintf(stdout, "Active Component Image Set Version String Length: %d\n",
            fw_params->active_comp_image_set_ver_str_len);
    fprintf(stdout, "Pending Component Image Set Version String Type: %d\n",
            fw_params->pending_comp_image_set_ver_str_type);
    fprintf(stdout, "Pending Component Image Set Version String Length: %d\n",
            fw_params->pending_comp_image_set_ver_str_len);

    buf_print("Active Component Image Set Version String:",
             (uint8_t *)fw_params->active_comp_image_set_ver_str,
             fw_params->active_comp_image_set_ver_str_len);

    buf_print("Pending Component Image Set Version String:",
             (uint8_t *)fw_params->pending_comp_image_set_ver_str,
             fw_params->pending_comp_image_set_ver_str_len);
}

void dump_firmware_comp_table(firmware_component_info_t *component_info)
{
    struct component_parameter_table *component_data = &component_info->comp_table;

    fprintf(stdout, "Component Classification: %d\n", component_data->comp_classification);
    fprintf(stdout, "Component Identifier: %d\n", component_data->comp_identifier);
    fprintf(stdout, "Component Classification Index: %d\n", component_data->comp_classification_index);

    fprintf(stdout, "Active Component Comparison Stamp: %d\n", component_data->active_comp_comparison_stamp);
    fprintf(stdout, "Active Component Version String Type: %d\n", component_data->active_comp_ver_str_type);
    fprintf(stdout, "Active Component Version String Length: %d\n", component_data->active_comp_ver_str_len);
    fprintf(stdout, "Active Component Release Date: %s\n", component_data->active_comp_release_date);
    buf_print("Active Component Version String:",
             (uint8_t *)component_info->active_comp_ver_str,
             component_data->active_comp_ver_str_len);
    fprintf(stdout, "Pending Component Comparison Stamp: %d\n", component_data->pending_comp_comparison_stamp);
    fprintf(stdout, "Pending Component Version String Type: %d\n", component_data->pending_comp_ver_str_type);
    fprintf(stdout, "Pending Component Version String Length: %d\n", component_data->pending_comp_ver_str_len);
    fprintf(stdout, "Pending Component Release Date: %s\n", component_data->pending_comp_release_date);
    buf_print("Pending Component Version String:",
             (uint8_t *)component_info->pending_comp_ver_str,
             component_data->pending_comp_ver_str_len);

    fprintf(stdout, "Component Activation Methods: %d\n", component_data->comp_activation_methods.value);
    fprintf(stdout, "Capabilities During Update: %d\n", component_data->capabilities_during_update.value);
}

int get_firmware_parameters(pldm_ctx_t *ctx)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }
    uint8_t instance_id = get_instance_id();
    struct pldm_msg request;
    int rc = encode_get_firmware_parameters_req(
        instance_id, &request, PLDM_GET_FIRMWARE_PARAMETERS_REQ_BYTES);

    if (rc)
    {
        fprintf(stderr, "Failed to encode get firmware parameters request: %d\n", rc);
        return rc;
    }
    uint8_t response_buf[PLDM_MAX_CMDS_PER_TYPE * 2];
    memset(response_buf, 0, PLDM_MAX_CMDS_PER_TYPE * 2);
    struct pldm_msg *response = (struct pldm_msg *)response_buf;
    // uint8_t response_len = 0x48; //PLDM_FWU_COMP_VER_STR_SIZE_MAX;
    uint16_t response_len = PLDM_MAX_CMDS_PER_TYPE * 2; // PLDM_FWU_COMP_VER_STR_SIZE_MAX;

    uint16_t req_len = 3 + PLDM_GET_FIRMWARE_PARAMETERS_REQ_BYTES;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&request,
                                      req_len,
                                      (uint8_t *)response, &response_len);

    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    struct variable_field active_comp_image_set_ver_str = {0};
    struct variable_field pending_comp_image_set_ver_str = {0};
    struct get_firmware_parameters_resp *resp_data = (struct get_firmware_parameters_resp *)response->payload;

    rc = decode_get_firmware_parameters_comp_img_set_resp(
        response, response_len, resp_data,
        &active_comp_image_set_ver_str,
        &pending_comp_image_set_ver_str);

    if (rc)
    {
        fprintf(stderr, "Failed to decode get firmware parameters response: %d\n", rc);
        return rc;
    }
    if (resp_data->completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Get firmware parameters failed with completion code: %d\n", resp_data->completion_code);
        pldm_fwu_print_error(ctx, __func__, resp_data->completion_code);
        return -1;
    }

    firmware_parameters_info_t *fw_params = &ctx->firmware_parameters;

    fw_params->comp_count = resp_data->comp_count;
    fw_params->capabilities_during_update = resp_data->capabilities_during_update;
    fw_params->active_comp_image_set_ver_str_type = resp_data->active_comp_image_set_ver_str_type;
    fw_params->active_comp_image_set_ver_str_len = resp_data->active_comp_image_set_ver_str_len;
    fw_params->pending_comp_image_set_ver_str_type = resp_data->pending_comp_image_set_ver_str_type;
    fw_params->pending_comp_image_set_ver_str_len = resp_data->pending_comp_image_set_ver_str_len;
    memcpy(fw_params->active_comp_image_set_ver_str,
           active_comp_image_set_ver_str.ptr,
           active_comp_image_set_ver_str.length);
    memcpy(fw_params->pending_comp_image_set_ver_str,
           pending_comp_image_set_ver_str.ptr,
           pending_comp_image_set_ver_str.length);

    dump_firmware_parameters(fw_params);

    uint8_t partial_response_length =
        sizeof(struct get_firmware_parameters_resp) +
        resp_data->active_comp_image_set_ver_str_len +
        resp_data->pending_comp_image_set_ver_str_len;

    struct component_parameter_table *comp_parameter_table = (struct component_parameter_table *)(response->payload + partial_response_length);
    uint8_t comp_parameter_table_len = response_len - partial_response_length;

    if (comp_parameter_table_len < sizeof(struct component_parameter_table))
    {
        fprintf(stderr, "Component parameter table length is too short: %d\n", comp_parameter_table_len);
        return -1;
    }

    if (resp_data->comp_count > PLDM_COMPONENTS_MAX)
    {
        fprintf(stderr, "Component count exceeds maximum allowed: %d\n", resp_data->comp_count);
        return -1;
    }

    for (int i = 0; i < resp_data->comp_count; i++)
    {
        struct component_parameter_table component_data;
        struct variable_field active_comp_ver_str;
        struct variable_field pending_comp_ver_str;

        rc = decode_get_firmware_parameters_comp_resp((uint8_t *)comp_parameter_table,
                                                      comp_parameter_table_len,
                                                      &component_data,
                                                      &active_comp_ver_str,
                                                      &pending_comp_ver_str);
        if (rc)
        {
            fprintf(stderr, "Get firmware parameters component table decode failed: %d\n", rc);
            return -1;
        }

        fw_params->component_info[i].comp_table.comp_classification = component_data.comp_classification;
        fw_params->component_info[i].comp_table.comp_identifier = component_data.comp_identifier;
        fw_params->component_info[i].comp_table.comp_classification_index = component_data.comp_classification_index;
        fw_params->component_info[i].comp_table.active_comp_comparison_stamp = component_data.active_comp_comparison_stamp;
        fw_params->component_info[i].comp_table.active_comp_ver_str_type = component_data.active_comp_ver_str_type;
        fw_params->component_info[i].comp_table.active_comp_ver_str_len = component_data.active_comp_ver_str_len;
        fw_params->component_info[i].comp_table.pending_comp_comparison_stamp = component_data.pending_comp_comparison_stamp;
        fw_params->component_info[i].comp_table.pending_comp_ver_str_type = component_data.pending_comp_ver_str_type;
        fw_params->component_info[i].comp_table.pending_comp_ver_str_len = component_data.pending_comp_ver_str_len;
        memcpy(fw_params->component_info[i].comp_table.active_comp_release_date,
               component_data.active_comp_release_date, sizeof(component_data.active_comp_release_date));
        memcpy(fw_params->component_info[i].comp_table.pending_comp_release_date,
               component_data.pending_comp_release_date, sizeof(component_data.pending_comp_release_date));
        fw_params->component_info[i].comp_table.capabilities_during_update = component_data.capabilities_during_update;
        fw_params->component_info[i].comp_table.comp_activation_methods = component_data.comp_activation_methods;

        memcpy(fw_params->component_info[i].active_comp_ver_str,
               active_comp_ver_str.ptr, active_comp_ver_str.length);
        memcpy(fw_params->component_info[i].pending_comp_ver_str,
               pending_comp_ver_str.ptr, pending_comp_ver_str.length);

        partial_response_length = sizeof(struct component_parameter_table) +
                                  active_comp_ver_str.length + pending_comp_ver_str.length;

        comp_parameter_table = (struct component_parameter_table *)((uint8_t *)comp_parameter_table + partial_response_length);
        comp_parameter_table_len -= partial_response_length;

        dump_firmware_comp_table(&fw_params->component_info[i]);
    }
    return rc;
}

int request_update(pldm_ctx_t *ctx, int component_id)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }
    uint8_t instance_id = get_instance_id();

    firmware_component_info_t *comp_info = &ctx->firmware_parameters.component_info[component_id];
    if (comp_info == NULL)
    {
        fprintf(stderr, "Invalid component info for component ID: %d\n", component_id);
        return -EINVAL;
    }

    uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

    struct pldm_msg *request = (struct pldm_msg *)tbuf;

    struct request_update_req req_data;
    req_data.max_transfer_size = 32;
    req_data.no_of_comp = 2;
    req_data.max_outstand_transfer_req = 1;
    req_data.pkg_data_len = 0;
    req_data.comp_image_set_ver_str_type = comp_info->comp_table.active_comp_ver_str_type;
    req_data.comp_image_set_ver_str_len = comp_info->comp_table.active_comp_ver_str_len;

    struct variable_field comp_img_set_ver_str;
    comp_img_set_ver_str.ptr = comp_info->active_comp_ver_str;
    comp_img_set_ver_str.length = comp_info->comp_table.active_comp_ver_str_len;

    uint8_t payload_length = sizeof(struct request_update_req) + comp_img_set_ver_str.length;
    if (payload_length > PLDM_MAX_CMDS_PER_TYPE)
    {
        fprintf(stderr, "Payload size exceeds maximum allowed: %u\n", payload_length);
        return -1;
    }

    int rc = encode_request_update_req(instance_id, request,
                                       payload_length,
                                       &req_data, &comp_img_set_ver_str);

    if (rc)
    {
        fprintf(stderr, "Failed to encode request update: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = PLDM_MAX_CMDS_PER_TYPE - 1;

    uint16_t req_len = 3 + payload_length;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)request,
                                      req_len,
                                      (uint8_t *)response, &response_len);

    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    fprintf(stdout, "%s: response length - %d\n", __func__, response_len);
    fprintf(stdout, "computed response length: %zu\n",
            sizeof(struct pldm_request_update_resp) + sizeof(struct pldm_msg_hdr));

    if (response_len != sizeof(struct pldm_request_update_resp) + sizeof(struct pldm_msg_hdr))
    {
        fprintf(stderr, "Invalid response length: %d\n", response_len);
        return -1;
    }

    uint8_t completion_code;
    uint16_t pkg_data_len;
    uint8_t *pkg_data = tbuf;

    rc = decode_request_update_resp(response, response_len - sizeof(struct pldm_msg_hdr),
                                    &completion_code, &pkg_data_len, pkg_data);

    if (rc)
    {
        fprintf(stderr, "Failed to decode request update response: %d\n", rc);
        return rc;
    }

    fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        pldm_fwu_print_error(ctx, __func__, rc);
        fprintf(stderr, "Request update failed with completion code: %d\n", completion_code);
        return -1;
    }
    fprintf(stdout, "Package Data Length: %d\n", pkg_data_len);
    if (pkg_data_len > 0)
    {
        fprintf(stdout, "Package Data: ");
        for (int i = 0; i < pkg_data_len; i++)
        {
            fprintf(stdout, "%02x ", pkg_data[i]);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "Request Update successful for component ID: %d\n", component_id);
    return rc;
}

int pass_component_table(pldm_ctx_t *ctx, int component_id)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }
    uint8_t instance_id = get_instance_id();

    firmware_component_info_t *comp_info = &ctx->firmware_parameters.component_info[component_id];
    if (comp_info == NULL)
    {
        fprintf(stderr, "Invalid component info for component ID: %d\n", component_id);
        return -EINVAL;
    }

    uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

    struct pldm_msg *request = (struct pldm_msg *)tbuf;

    struct pass_component_table_req comp_table_data;
    comp_table_data.transfer_flag = PLDM_END;
    comp_table_data.comp_classification = comp_info->comp_table.comp_classification;
    comp_table_data.comp_identifier = comp_info->comp_table.comp_identifier;
    comp_table_data.comp_classification_index = comp_info->comp_table.comp_classification_index;
    // comp_table_data.comp_comparison_stamp = comp_info->comp_table.active_comp_comparison_stamp;
    comp_table_data.comp_comparison_stamp = 0xFFFFFFFF;

    comp_table_data.comp_ver_str_type = comp_info->comp_table.active_comp_ver_str_type;
    comp_table_data.comp_ver_str_len = comp_info->comp_table.active_comp_ver_str_len;

    struct variable_field comp_ver_str;
    comp_ver_str.ptr = comp_info->active_comp_ver_str;
    comp_ver_str.length = comp_info->comp_table.active_comp_ver_str_len;

    uint8_t payload_length = sizeof(struct pass_component_table_req) + comp_ver_str.length;
    if (payload_length > PLDM_MAX_CMDS_PER_TYPE)
    {
        fprintf(stderr, "Payload size exceeds maximum allowed: %u\n", payload_length);
        return -1;
    }

    int rc = encode_pass_component_table_req(instance_id,
                                             request,
                                             payload_length,
                                             &comp_table_data,
                                             &comp_ver_str);

    if (rc)
    {
        fprintf(stderr, "Failed to encode pass component table request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = PLDM_MAX_CMDS_PER_TYPE - 1;

    uint16_t req_len = 3 + payload_length;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)request,
                                      req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    if (response_len != (sizeof(struct pass_component_table_resp) + sizeof(struct pldm_msg_hdr)))
    {
        fprintf(stderr, "Invalid response length: %d\n", response_len);
        return -1;
    }
    uint8_t completion_code;
    uint8_t comp_resp;
    uint8_t comp_resp_code;

    uint8_t pl_size = response_len - sizeof(struct pldm_msg_hdr);
    fprintf(stdout, "%s: payload length - %d\n", __func__, pl_size);
    rc = decode_pass_component_table_resp(response, pl_size,
                                          &completion_code, &comp_resp,
                                          &comp_resp_code);
    if (rc)
    {
        fprintf(stderr, "Failed to decode pass component table response: %d\n", rc);
        return rc;
    }
    fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        pldm_fwu_print_error(ctx, __func__, completion_code);
        fprintf(stderr, "Pass component table failed with completion code: %d\n", completion_code);
        return -1;
    }
    fprintf(stdout, "Component Response: %d\n", comp_resp);

    return rc;
}

int pldm_inventory_update(pldm_ctx_t *ctx)
{
    int rc = 0;
    if (ctx == NULL)
    {
        fprintf(stdout, "ctx is NULL\n");
        return -EINVAL;
    }

    if (ctx->pldm_types_count == 0)
    {
        fprintf(stdout, "No PLDM types supported\n");
        return -EINVAL;
    }
    uint8_t descriptor_count = 0;
    uint32_t device_identifiers_len = 0;

    uint8_t data[16];
    uint8_t *descriptor_data = data;

    rc = query_device_id(ctx, &descriptor_data,
                         &device_identifiers_len,
                         &descriptor_count);

    if (rc)
    {
        fprintf(stdout, "query_device_identifiers failed with error code: %d\n", rc);
        return rc;
    }
    if (descriptor_count == 0)
    {
        fprintf(stdout, "No descriptors found\n");
        return -EINVAL;
    }
    if (device_identifiers_len == 0)
    {
        fprintf(stdout, "No device identifiers found\n");
        return -EINVAL;
    }

    ctx->descriptor_count = descriptor_count;
    memcpy(&ctx->pldm_device_id, descriptor_data, device_identifiers_len);
    printf("Descriptor Count: %d\n", ctx->descriptor_count);
    printf("Device Identifiers Length: %d\n", ctx->pldm_device_id.descriptor_len);
    printf("Descriptor Type: %d\n", ctx->pldm_device_id.descriptor_type);
    buf_print("Device Identifiers", ctx->pldm_device_id.descriptor_data, ctx->pldm_device_id.descriptor_len);

    rc = get_firmware_parameters(ctx);
    return rc;
}

int update_component(pldm_ctx_t *ctx, int component_id)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    component_image_info_t *comp_info = &ctx->firmware_package.component_images_info.component_images[component_id];
    if (comp_info == NULL)
    {
        fprintf(stderr, "Invalid component info for component ID: %d\n", component_id);
        return -EINVAL;
    }
    

    uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;

    uint8_t instance_id = get_instance_id();


    struct update_component_req update_component_req = 
    {
        .comp_classification = comp_info->component_classification,
        .comp_identifier = comp_info->component_identifier,
        .comp_classification_index = 0,
        .comp_comparison_stamp = comp_info->component_comparison_stamp,
        .comp_ver_str_type = comp_info->component_version_string_type,
        .comp_ver_str_len = comp_info->component_version_string_length,
        .comp_image_size = comp_info->component_size,
        .update_option_flags.value = 0 
    };

    struct variable_field comp_ver_str = 
    {
        .ptr = comp_info->component_version_string,
        .length = comp_info->component_version_string_length
    };

    uint8_t payload_length = sizeof(struct update_component_req) + comp_ver_str.length;
    
    int rc = encode_update_component_req(instance_id, request,
				payload_length,
				&update_component_req,
				&comp_ver_str);
    if (rc)
    {
        fprintf(stderr, "Failed to encode update component request: %d\n", rc);
        return rc;
    }
    uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = PLDM_MAX_CMDS_PER_TYPE - 1; 
    uint16_t req_len = 3 + payload_length;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)request,
                                      req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }
    if (response_len != sizeof(struct update_component_resp) + sizeof(struct pldm_msg_hdr))
    {
        fprintf(stderr, "Invalid response length: %d\n", response_len);
        return -1;
    }
    uint8_t completion_code;
    uint8_t comp_compatability_resp;
	uint8_t comp_compatability_resp_code;
	bitfield32_t update_option_flags_enabled;
	uint16_t estimated_time_req_fd;

    rc = decode_update_component_resp(response, response_len - sizeof(struct pldm_msg_hdr),
                    &completion_code,
                    &comp_compatability_resp,
				    &comp_compatability_resp_code,
				    &update_option_flags_enabled,
				    &estimated_time_req_fd);

    if (rc)
    {
        fprintf(stderr, "Failed to decode update component response: %d\n", rc);
        return rc;
    }
    fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        pldm_fwu_print_error(ctx, __func__, completion_code);
        fprintf(stderr, "Update component failed with completion code: %d\n", completion_code);
        return -1;
    }

    initialize_fw_update(32, comp_info->component_size);
    
    fprintf(stdout, "Component Compatibility Response: %d\n", comp_compatability_resp);
    fprintf(stdout, "Component Compatibility Response Code: %d\n", comp_compatability_resp_code);
    fprintf(stdout, "Update Option Flags Enabled: %d\n", update_option_flags_enabled.value);
    fprintf(stdout, "Estimated Time Required for Firmware Download: %d\n", estimated_time_req_fd);
    
    return 0;
}

void pldm_transfer_print_error(uint8_t rc) 
{
	enum pldm_fwu_transfer_result  transfer_result = (enum pldm_fwu_transfer_result)rc;
	
	switch (transfer_result) {
	case PLDM_FWU_TRASFER_SUCCESS:
		printf("Transfer completed successfully.\n");
		break;
	case PLDM_FWU_TRANSFER_COMPLETE_WITH_ERROR:
		printf("Transfer completed with an error.\n");
		break;
	case PLDM_FWU_FD_ABORTED_TRANSFER:
		printf("Transfer was aborted by the firmware device.\n");
		break;
	default:
		if (transfer_result >= PLDM_FWU_VENDOR_TRANSFER_RESULT_RANGE_MIN &&
		    transfer_result <= PLDM_FWU_VENDOR_TRANSFER_RESULT_RANGE_MAX) {
			printf("Vendor-specific transfer result: 0x%02x\n", transfer_result);
		} else {
			printf("Unknown transfer result: 0x%02x\n", transfer_result);
		}
		break;
	}	
}

int trasfer_complete(pldm_ctx_t *ctx, struct pldm_msg *request, uint8_t payload_length)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t transfer_result;

    int rc = decode_transfer_complete_req(request, &transfer_result);
    if (rc)
    {
        fprintf(stderr, "Failed to decode transfer complete request: %d\n", rc);
        return rc;
    }
    fprintf(stdout, "Transfer Result: %d\n", transfer_result);
    if (transfer_result != PLDM_SUCCESS)
    {
        pldm_transfer_print_error(transfer_result);
        fprintf(stderr, "Transfer complete failed with result: %d\n", transfer_result);
        return -1;
    }

    uint8_t instance_id = get_instance_id();
    struct pldm_msg msg = {0};
    rc = encode_transfer_complete_resp(instance_id, PLDM_SUCCESS, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode transfer complete response: %d\n", rc);
        return rc;
    }

    uint8_t resp_len = sizeof(struct pldm_msg);
    rc = pldm_send_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                 (uint8_t *)&msg,
                                 resp_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    return 0;
}

void pldm_verify_print_error(uint8_t rc)
{
    enum pldm_fwu_verify_result verify_result = (enum pldm_fwu_verify_result)rc;

    switch (verify_result)
    {
    case PLDM_FWU_VERIFY_SUCCESS:
        printf("Verify completed successfully.\n");
        break;
        case PLDM_FWU_VERIFY_COMPLETED_WITH_FAILURE:
        printf("Verify completed with failure.\n");
        break;
    case PLDM_FWU_VERIFY_COMPLETED_WITH_ERROR:
        printf("Verify completed with error.\n");
        break;
    case PLDM_FWU_VENDOR_SPEC_STATUS_RANGE_MIN ... PLDM_FWU_VENDOR_SPEC_STATUS_RANGE_MAX:
        printf("Vendor-specific verify result: 0x%02x\n", verify_result);
        break;

    default:
        printf("Unknown verify result: 0x%02x\n", verify_result);
        break;
    }
}

int verify_complete(pldm_ctx_t *ctx, struct pldm_msg *request,
                    uint8_t payload_length)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t verify_result = 0;
    int rc = decode_verify_complete_req(request, &verify_result);
    if (rc)
    {
        fprintf(stderr, "Failed to decode verify complete request: %d\n", rc);
        return rc;
    }

    if (verify_result != PLDM_SUCCESS)
    {
        pldm_verify_print_error(verify_result);
        fprintf(stderr, "Verify complete failed with result: %d\n", verify_result);
        return -1;
    }

    uint8_t instance_id = get_instance_id();
    struct pldm_msg msg = {0};
    rc = encode_verify_complete_resp(instance_id, PLDM_SUCCESS, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode verify complete response: %d\n", rc);
        return rc;
    }
    uint8_t resp_len = sizeof(struct pldm_msg);
    rc = pldm_send_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                 (uint8_t *)&msg,
                                 resp_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    fprintf(stdout, "Verify complete successful\n");
    return 0;
}

void pldm_apply_print_error(uint8_t rc)
{
    enum pldm_fwu_apply_result apply_result = (enum pldm_fwu_apply_result)rc;

    switch (apply_result)
    {
    case PLDM_FWU_APPLY_SUCCESS:
        printf("Apply completed successfully.\n");
        break;
    case PLDM_FWU_APPLY_SUCCESS_WITH_ACTIVATION_METHOD:
        printf("Apply completed successfully with activation method.\n");
        break;
    case PLDM_FWU_APPLY_COMPLETED_WITH_FAILURE:
        printf("Apply completed with failure.\n");
        break;
    case PLDM_FWU_VENDOR_APPLY_RESULT_RANGE_MIN ... PLDM_FWU_VENDOR_APPLY_RESULT_RANGE_MAX:
        printf("Vendor-specific apply result: 0x%02x\n", apply_result);
        break;        
    default:
        printf("Unknown apply result: 0x%02x\n", apply_result);
        break;
    }
}

int apply_complete(pldm_ctx_t *ctx, struct pldm_msg *request,
                   uint8_t payload_length)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t apply_result = 0;
    bitfield16_t comp_activation_methods_modification;
    comp_activation_methods_modification.value = 0;
    payload_length -= sizeof(struct pldm_msg_hdr);

    int rc = decode_apply_complete_req(request, payload_length, &apply_result,
                                       &comp_activation_methods_modification);
    if (rc)
    {
        fprintf(stderr, "Failed to decode apply complete request: %d\n", rc);
        return rc;
    }
    if (apply_result != PLDM_SUCCESS)
    {
        pldm_apply_print_error(apply_result);
        fprintf(stderr, "Apply complete failed with result: %d\n", apply_result);
        return -1;
    }
    fprintf(stdout, "Apply Result: %d\n", apply_result);

    uint8_t instance_id = get_instance_id();
    struct pldm_msg msg = {0};
    rc = encode_apply_complete_resp(instance_id, PLDM_SUCCESS, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode apply complete response: %d\n", rc);
        return rc;
    }

    uint8_t resp_len = sizeof(struct pldm_msg);
    rc = pldm_send_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                 (uint8_t *)&msg,
                                 resp_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    return rc;
}

int activate_firmware(pldm_ctx_t *ctx)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t instance_id = get_instance_id();

    struct pldm_msg msg = {0};
    uint8_t payload_length = 1;
    bool8_t self_contained_activation_req = true;

    int rc = encode_activate_firmware_req(instance_id, &msg,
                                          payload_length,
                                          self_contained_activation_req);
    if (rc)
    {
        fprintf(stderr, "Failed to encode activate firmware request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = PLDM_MAX_CMDS_PER_TYPE - 1;

    uint16_t req_len = 3 + payload_length;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&msg,
                                      req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code;
    uint16_t estimated_time_activation;

    rc = decode_activate_firmware_resp(response,
                                       response_len - sizeof(struct pldm_msg_hdr),
                                       &completion_code,
                                       &estimated_time_activation);
    if (rc)
    {
        fprintf(stderr, "Failed to decode activate firmware response: %d\n", rc);
        return rc;
    }
    fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Activate firmware failed with completion code: %d\n", completion_code);
        return -1;
    }
    fprintf(stdout, "Estimated Time for Activation: %d seconds\n", estimated_time_activation);

    return 0;
}

int get_status(pldm_ctx_t *ctx,
               uint8_t *current_state,
               uint8_t *previous_state,
               uint8_t *aux_state,
               uint8_t *aux_state_status,
               uint8_t *progress_percent,
               uint8_t *reason_code,
               bitfield32_t *update_option_flags_enabled)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t instance_id = get_instance_id();

    struct pldm_msg msg = {0};

    int rc = encode_get_status_req(instance_id, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get status request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = PLDM_MAX_CMDS_PER_TYPE - 1;

    uint16_t req_len = 3;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&msg,
                                      req_len,
                                      (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code;

    rc = decode_get_status_resp(response, response_len - sizeof(struct pldm_msg_hdr),
                                &completion_code, current_state,
                                previous_state,
                                aux_state,
                                aux_state_status,
                                progress_percent,
                                reason_code,
                                update_option_flags_enabled);
    if (rc)
    {
        fprintf(stderr, "Failed to decode get status response: %d\n", rc);
        return rc;
    }
    fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Get status failed with completion code: %d\n", completion_code);
        return -1;
    }

    return 0;
}

int cancel_update_component(pldm_ctx_t *ctx, int component_id)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t instance_id = get_instance_id();

    struct pldm_msg msg = {0};
    int rc = encode_cancel_update_component_req(instance_id, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode cancel update request: %d\n", rc);
        return rc;
    }

    struct pldm_msg response = {0};
    uint16_t req_len = 3;
    uint16_t response_len = sizeof(struct pldm_msg) + 8;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&msg,
                                      req_len,
                                      (uint8_t *)&response, &response_len);

    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code;

    rc = decode_cancel_update_component_resp(&response,
                                             response_len - sizeof(struct pldm_msg_hdr),
                                             &completion_code);
    if (rc)
    {
        fprintf(stderr, "Failed to decode cancel update component response: %d\n", rc);
        return rc;
    }
    fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Cancel update component failed with completion code: %d\n", completion_code);
        return -1;
    }
    return 0;
}

int cancel_update(pldm_ctx_t *ctx)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }
    uint8_t instance_id = get_instance_id();

    struct pldm_msg request = {0};
    int rc = encode_cancel_update_req(instance_id, &request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode cancel update request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
    memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = PLDM_MAX_CMDS_PER_TYPE - 1;

    uint16_t req_len = 3;
    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                      (uint8_t *)&request,
                                      req_len,
                                      (uint8_t *)response, &response_len);

    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    if (response_len != sizeof(struct cancel_update_resp) + sizeof(struct pldm_msg_hdr))
    {
        fprintf(stderr, "Invalid response length: %d\n", response_len);
        return -1;
    }

    uint8_t completion_code;
    bool8_t non_functioning_component_indication;
    bitfield64_t non_functioning_component_bitmap;

    rc = decode_cancel_update_resp(response, response_len - sizeof(struct pldm_msg_hdr),
                                   &completion_code,
                                   &non_functioning_component_indication,
                                   &non_functioning_component_bitmap);
    if (rc)
    {
        fprintf(stderr, "Failed to decode cancel update response: %d\n", rc);
        return rc;
    }

    fprintf(stdout, "Completion Code: %d\n", completion_code);
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Cancel update failed with completion code: %d\n", completion_code);
        return -1;
    }

    fprintf(stdout, "Cancel update successful\n");

    return rc;
}

void pldm_get_status_print(pldm_ctx_t *ctx)
{
    uint8_t current_state = ctx->fwu_status.current_state;
    uint8_t previous_state = ctx->fwu_status.previous_state;
    uint8_t aux_state = ctx->fwu_status.aux_state;
    uint8_t aux_state_status = ctx->fwu_status.aux_state_status;
    uint8_t progress_percent = ctx->fwu_status.progress_percent;
    uint8_t reason_code = ctx->fwu_status.reason_code;
    //bitfield32_t update_option_flags_enabled = ctx->fwu_state.update_option_flags_enabled;
    
    if (current_state == FD_IDLE)
    {
        fprintf(stdout, "Firmware update is idle.\n");
    }
    else if (current_state == FD_LEARN_COMPONENTS)
    {
        fprintf(stdout, "Firmware update is learning components.\n");
    }
    else if (current_state == FD_READY_XFER)
    {
        fprintf(stdout, "Firmware update is ready for transfer.\n");
    }
    else if (current_state == FD_DOWNLOAD)
    {
        fprintf(stdout, "Firmware update is downloading.\n");
    }
    else if (current_state == FD_VERIFY)
    {
        fprintf(stdout, "Firmware update is verifying.\n");
    }
    else if (current_state == FD_APPLY)
    {
        fprintf(stdout, "Firmware update is applying.\n");
    }
    else if (current_state == FD_ACTIVATE)
    {
        fprintf(stdout, "Firmware update is activating.\n");
    }
    else
    {
        fprintf(stdout, "Unknown firmware update state: %d\n", current_state);
    }

    if (previous_state == FD_IDLE)
    {
        fprintf(stdout, "Previous state was idle.\n");
    }
    else if (previous_state == FD_LEARN_COMPONENTS)
    {
        fprintf(stdout, "Previous state was learning components.\n");
    }
    else if (previous_state == FD_READY_XFER)
    {
        fprintf(stdout, "Previous state was ready for transfer.\n");
    }
    else if (previous_state == FD_DOWNLOAD)
    {
        fprintf(stdout, "Previous state was downloading.\n");
    }
    else if (previous_state == FD_VERIFY)
    {
        fprintf(stdout, "Previous state was verifying.\n");
    }
    else if (previous_state == FD_APPLY)
    {
        fprintf(stdout, "Previous state was applying.\n");
    }
    else if (previous_state == FD_ACTIVATE)
    {
        fprintf(stdout, "Previous state was activating.\n");
    }
    else
    {
        fprintf(stdout, "Unknown previous state: %d\n", previous_state);
    }

    if (aux_state == FD_OPERATION_IN_PROGRESS)
    {
        fprintf(stdout, "Auxiliary state status is in progress.\n");
    }
    else if (aux_state == FD_OPERATION_SUCCESSFUL)
    {
        fprintf(stdout, "Auxiliary state status is successful.\n");
    }
    else if (aux_state == FD_OPERATION_FAILED)
    {
        fprintf(stdout, "Auxiliary state status is failed.\n");
    }
    else if (aux_state == FD_WAIT)
    {
        fprintf(stdout, "Auxiliary state status is waiting.\n");
    }
    else
    {
        fprintf(stdout, "Unknown auxiliary state status: %d\n", aux_state);
    }

    if (aux_state_status == FD_AUX_STATE_IN_PROGRESS_OR_SUCCESS)
    {
        fprintf(stdout, "Auxiliary state status is in progress or successful.\n");
    }
    else if (aux_state_status == FD_TIMEOUT)
    {
        fprintf(stdout, "Auxiliary state status is timeout.\n");
    }
    else if (aux_state_status == FD_GENERIC_ERROR)
    {
        fprintf(stdout, "Auxiliary state status is generic error.\n");
    }
    else if (aux_state_status >= FD_VENDOR_DEFINED_STATUS_CODE_START &&
             aux_state_status <= FD_VENDOR_DEFINED_STATUS_CODE_END)
    {
        fprintf(stdout, "Auxiliary state status is vendor defined: %d\n", aux_state_status);
    }
    else
    {
        fprintf(stdout, "Unknown auxiliary state status code: %d\n", aux_state_status);
    }

    if (progress_percent < 0 || progress_percent > 100)
    {
        fprintf(stdout, "Progress percent is out of range: %d\n", progress_percent);
    }
    else
    {
        fprintf(stdout, "Progress percent is: %d%%\n", progress_percent);
    }

    if (reason_code == FD_INITIALIZATION)
    {
        fprintf(stdout, "Reason code is initialization.\n");
    }
    else if (reason_code == FD_ACTIVATE_FW_RECEIVED)
    {
        fprintf(stdout, "Reason code is activate firmware received.\n");
    }
    else if (reason_code == FD_CANCEL_UPDATE_RECEIVED)
    {
        fprintf(stdout, "Reason code is cancel update received.\n");
    }
    else if (reason_code == FD_TIMEOUT_LEARN_COMPONENT)
    {
        fprintf(stdout, "Reason code is timeout learning components.\n");
    }
    else if (reason_code == FD_TIMEOUT_READY_XFER)
    {
        fprintf(stdout, "Reason code is timeout ready for transfer.\n");
    }
    else if (reason_code == FD_TIMEOUT_DOWNLOAD)
    {
        fprintf(stdout, "Reason code is timeout downloading.\n");
    }
    else if (reason_code >= FD_STATUS_VENDOR_DEFINED_MIN &&
             reason_code <= FD_STATUS_VENDOR_DEFINED_MAX)
    {
        fprintf(stdout, "Reason code is vendor defined: %d\n", reason_code);
    }
    else
    {
        fprintf(stdout, "Unknown reason code: %d\n", reason_code);
    }
}

int pldm_fd_status_update(pldm_ctx_t *ctx)
{
    fprintf(stdout, "%s\n", __func__);
    if (ctx == NULL)
    {
        fprintf(stdout, "ctx is NULL\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stdout, "mctp_ctx is NULL\n");
        return -EINVAL;
    }

    uint8_t current_state;
    uint8_t previous_state;
    uint8_t aux_state;
    uint8_t aux_state_status;
    uint8_t progress_percent;
    uint8_t reason_code;
    bitfield32_t update_option_flags_enabled;

    int rc = get_status(ctx,
                        &current_state,
                        &previous_state,
                        &aux_state,
                        &aux_state_status,
                        &progress_percent,
                        &reason_code,
                        &update_option_flags_enabled);
    if (rc)
    {
        fprintf(stdout, "get status failed with error code: %d\n", rc);
        return rc;
    }

    ctx->fwu_status.current_state = current_state;
    ctx->fwu_status.previous_state = previous_state;
    ctx->fwu_status.aux_state = aux_state;
    ctx->fwu_status.aux_state_status = aux_state_status;
    ctx->fwu_status.progress_percent = progress_percent;
    ctx->fwu_status.reason_code = reason_code;
    ctx->fwu_status.update_option_flags_enabled = update_option_flags_enabled;

    return rc;
}

int get_data(pldm_ctx_t *ctx, size_t offset, uint8_t *data, size_t length)
{
    FILE *fp = ctx->firmware_package.fp;
    if (fp == NULL) {
        fprintf(stderr, "Firmware package file pointer is NULL\n");
        return -EINVAL;
    }

    component_image_info_t *comp_info = &ctx->firmware_package.component_images_info.component_images[0];
    if (offset + length > comp_info->component_size) {
        fprintf(stderr, "Requested data exceeds component size: offset %lu, length %lu, component size %u\n",
                offset, length, comp_info->component_size);
        return -EINVAL;
    }
    if (fseek(fp, comp_info->component_location_offset + offset, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to seek to offset %zu in package file\n", offset);
        return -EIO;
    }
    size_t bytes_read = fread(data, 1, length, fp);
    if (bytes_read < 0) {
        if (ferror(fp)) {
            fprintf(stderr, "Error reading from package file\n");
            return -EIO;
        }
    }
    
    return length;
}


// Pass the message which was read
int send_firmware_data(pldm_ctx_t *ctx, int component_id)
{
    fprintf(stderr, "%s\n", __func__);

    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid context\n");
        return -EINVAL;
    }
    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    bool first_request = true;
    int rc = 0;
    uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
    struct pldm_msg *request = (struct pldm_msg *)tbuf;

    component_image_info_t *comp_info = &ctx->firmware_package.component_images_info.component_images[component_id];
    size_t image_size = comp_info->component_size;

    ctx->state = PLDM_FWU_STATE_DOWNLOADING;
    fprintf(stdout, "Starting firmware data transfer for component ID: %d\n", component_id);

    while (true)
    {
        if (first_request)
        {
            fprintf(stdout, "image size: %zu\n", image_size);
            usleep(5000 * 1000); // 5 seconds delay before first get status
            first_request = false;
        }

        memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
        uint16_t req_len = PLDM_MAX_CMDS_PER_TYPE - 1;

        rc = pldm_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)request, &req_len);

        uint32_t offset;
        uint32_t length;

        if (rc < 0)
        {
            fprintf(stderr, "Failed to receive message: %d\n", rc);
            return rc;
        }

        if (request->hdr.command != PLDM_REQUEST_FIRMWARE_DATA)
        {
            //fprintf(stderr, "Received unexpected message type: %d\n", ((uint8_t *)request)[3]);
            if (request->hdr.command == PLDM_TRANSFER_COMPLETE)
            {
                fprintf(stdout, "Received Transfer Complete message\n");
                rc = trasfer_complete(ctx, request, req_len);
                if (rc < 0)
                {
                    fprintf(stderr, "Transfer complete failed: %d\n", rc);
                    return rc;
                }
                usleep(5000 * 1000); // 5 seconds delay before next get status
                fprintf(stdout, "Transfer complete successful\n"); 
                continue; // Continue to the next iteration to check for more messages
            }

            if (request->hdr.command == PLDM_VERIFY_COMPLETE)
            {
                fprintf(stdout, "Received Verify Complete message\n");
                rc = verify_complete(ctx, request, req_len);
                if (rc < 0)
                {
                    fprintf(stderr, "Verify complete failed: %d\n", rc);
                    return rc;
                }
                ctx->state = PLDM_FWU_STATE_VERIFY_COMPLETED;
                usleep(5000 * 1000); // 5 seconds delay before next get status
                fprintf(stdout, "Verify complete successful\n");
                continue; // Continue to the next iteration to check for more messages
            }

            if (request->hdr.command == PLDM_APPLY_COMPLETE)
            {
                fprintf(stdout, "Received Apply Complete message\n");
                rc = apply_complete(ctx, request, req_len);
                if (rc < 0)
                {
                    fprintf(stderr, "Apply complete failed: %d\n", rc);
                    return rc;
                }
                ctx->state = PLDM_FWU_STATE_APPLY_COMPLETED;
                fprintf(stdout, "Apply complete successful\n");
                return rc;
            }
        }
        
        int rc = decode_request_firmware_data_req(request, req_len - sizeof(struct pldm_msg_hdr),
                                                  &offset, &length);
        if (rc)
        {
            if (rc == PLDM_ERROR_INVALID_DATA)
            {
                fprintf(stderr, "Invalid firmware data request\n");
                return -EINVAL;
            }
            // If we reach here, it means we got an error that is not PLDM_ERROR_NO_DATA
            if (rc == PLDM_ERROR_INVALID_LENGTH)
            {
                fprintf(stderr, "Invalid length in request firmware data request: %d\n", rc);
                return -EINVAL;
            } 
        
            // Handle other errors
            if (rc < 0)
            {
                fprintf(stderr, "Failed to decode request firmware data request: %d\n", rc);
                return rc;
            }
            fprintf(stderr, "Failed to decode request firmware data request: %d\n", rc);

            return rc;
        }
        fprintf(stdout, "Offset: %d, Length: %d\n", offset, length);
        if (length == 0)
        {
            fprintf(stderr, "Invalid length: %d\n", length);
            return -EINVAL;
        }

        uint8_t data[PLDM_MAX_CMDS_PER_TYPE];

        rc = get_data(ctx, offset, data, length);
        if (rc < 0)
        {
            fprintf(stderr, "Failed to get firmware data: %d\n", rc);
            return rc;
        }

        uint8_t instance_id = get_instance_id();
        struct variable_field data_field;
        data_field.ptr = data;
        data_field.length = length;

        uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
        memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);
        struct pldm_msg *msg = (struct pldm_msg *)tbuf;

        rc = encode_request_firmware_data_resp(instance_id, msg,
                                               length + 1, PLDM_SUCCESS, &data_field);
        if (rc)
        {
            fprintf(stderr, "Failed to encode request firmware data response: %d\n", rc);
            return rc;
        }

        uint8_t resp_len = sizeof(struct pldm_msg) + length;
        rc = pldm_send_msg_over_mctp(ctx->mctp_ctx, ctx->tid,
                                     (uint8_t *)msg,
                                     resp_len);
        if (rc)
        {
            fprintf(stderr, "Failed to send/receive message: %d\n", rc);
            return rc;
        }
        image_size -= length;
        if (image_size == 0)
        {
             ctx->state = PLDM_FWU_STATE_TRANSFER_COMPLETED;
            fprintf(stdout, "Firmware data transfer complete for component ID: %d\n", component_id);
            usleep(5000 * 1000);
        }
    }

    return rc;
}

int fwupdate(pldm_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        fprintf(stdout, "ctx is NULL\n");
        return -EINVAL;
    }
    if (ctx->firmware_package.fp == NULL)
    {
        fprintf(stdout, "Firmware package file pointer is NULL\n");
        return -EINVAL;
    }

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stdout, "mctp_ctx is NULL\n");
        return -EINVAL;
    }

    if (ctx->state != PLDM_FWU_STATE_IDLE)
    {
        fprintf(stdout, "Firmware update is already in progress\n");
        return -EBUSY;
    }
    ctx->state = PLDM_FWU_STATE_PREPARING;

    int rc = pldm_inventory_update(ctx);
    if (!rc)
    {
        fprintf(stdout, "pldm inventory update is successful\n");
    }
    else
    {
        fprintf(stdout, "pldm inventory update failed\n");
        free(ctx->firmware_package.fp);
        ctx->firmware_package.fp = NULL;
        return rc;
    }

    usleep(100 * 1000);
    rc = request_update(ctx, 0);
    if (rc)
    {
        fprintf(stdout, "request update failed with error code: %d\n", rc);
        return rc;
    }
    else
    {
        fprintf(stdout, "request update is successful\n");
    }

    rc = pass_component_table(ctx, 0);
    if (rc)
    {
        fprintf(stdout, "pass component table failed with error code: %d\n", rc);
        return rc;
    }
    else
    {
        fprintf(stdout, "pass component table is successful\n");
    }

    rc = update_component(ctx, 0);
    if (rc)
    {
        fprintf(stdout, "update component failed with error code: %d\n", rc);
        return rc;
    }
    else
    {
        fprintf(stdout, "update component is successful\n");
    }

    ctx->state = PLDM_FWU_STATE_DOWNLOADING;
    rc = send_firmware_data(ctx, 0);
    if (rc)
    {
        fprintf(stdout, "send firmware data failed with error code: %d\n", rc);
        rc = cancel_update_component(ctx, 0);
        if (rc)
        {
            fprintf(stdout, "cancel update component failed with error code: %d\n", rc);
            return rc;
        }
        else
        {
            fprintf(stdout, "cancel update component is successful\n");
        }

        rc = cancel_update(ctx);
        if (rc)
        {
            fprintf(stdout, "cancel update failed with error code: %d\n", rc);
            return rc;
        }
        else
        {
            fprintf(stdout, "cancel update is successful\n");
        }

        return -EIO;
    }
    else
    {
        fprintf(stdout, "send firmware data is successful\n");
         usleep(5000 * 1000); // 5 seconds delay before first get status
        rc = activate_firmware(ctx);
        if (rc)
        {
            fprintf(stdout, "activate firmware failed with error code: %d\n", rc);
            return rc;
        }
        else
        {
            fprintf(stdout, "activate firmware is successful\n");
        }
        pldm_fd_status_update(ctx);
        pldm_get_status_print(ctx);
    }

    return 0;
}
