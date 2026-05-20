#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "../libpldm/base.h"
#include "config.h"
#include "pldm.h"
#include "../mctp/mctp.h"
#include "../libpldm/platform.h"
#include "../libpldm/pdr.h"

/* @brief Sets the operational state and event message enable for a numeric sensor.
 * @param ctx Pointer to the PLDM context.
 * @param sensor_id The ID of the numeric sensor to set.
 * @param sensor_operational_state The operational state to set for the sensor.
 * @param sensor_event_message_enable The event message enable state for the sensor.
 * @return 0 on success, negative error code on failure.
 */
int set_numeric_sensor_enable(pldm_ctx_t *ctx, uint8_t sensor_id,
                             uint8_t sensor_operational_state,
                             uint8_t sensor_event_message_enable)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_set_numeric_sensor_enable_req);

    int instance_id = get_instance_id();
    int rc = encode_set_numeric_sensor_enable_req(
        instance_id, sensor_id,
        sensor_operational_state,
        sensor_event_message_enable, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode set numeric sensor enable request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[MCTP_MTU_SIZE];
    memset(rbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = MCTP_MTU_SIZE;

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)request, req_len, (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code = response->payload[0];
    if (completion_code != PLDM_SUCCESS)
    {
        // Handle error based on completion code
        switch (completion_code)
        {
        case PLDM_PLATFORM_INVALID_SENSOR_ID:
            fprintf(stderr, "Invalid Sensor ID in request\n");
            return -EINVAL;
        case PLDM_PLATFORM_INVALID_STATE_VALUE:
            fprintf(stderr, "Invalid Operational State in request\n");
            return -EIO;
        case PLDM_PLATFORM_SET_EFFECTER_UNSUPPORTED_SENSORSTATE:
            fprintf(stderr, "Event Generation not Supported\n");
            return -EINVAL;
        default:
            fprintf(stderr, "Error completion code: %d\n", completion_code);
            return -EIO;
        }
        fprintf(stderr, "Set numeric sensor enable failed with completion code: %d\n", completion_code);
        return -EIO;
    }

    // Successfully set the numeric sensor enable state
    //fprintf(stdout, "Numeric sensor %d enabled with operational state %d and event message enable %d\n",
    //       sensor_id, sensor_operational_state, sensor_event_message_enable);

    return PLDM_SUCCESS; // Success
}

int amc_sensors_enable_all(void *pctx)
{
    pldm_ctx_t *ctx = (pldm_ctx_t *)pctx;

    if (ctx == NULL || ctx->mctp_ctx == NULL || ctx->platform_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context or Platform Context\n");
        return -EINVAL;
    }

    // Enable all numeric sensors
    for (int i = 0; i < 7 /* ctx->platform_ctx->numeric_sensor_record_count */; i++)
    {
        struct pldm_numeric_sensor_value_pdr *pdr = ctx->platform_ctx->numeric_sensor_pdr[i];
        if (pdr == NULL)
        {
            fprintf(stderr, "No more numeric sensor PDR records found\n");
            break;
        }
        int rc = set_numeric_sensor_enable(ctx, pdr->sensor_id, PLDM_SENSOR_ENABLED, PLDM_NO_EVENT_GENERATION);
        if (rc)
        {
            fprintf(stderr, "Failed to enable numeric sensor %d: %d\n", pdr->sensor_id, rc);
            return rc;
        }
    }

    return PLDM_SUCCESS; // Success
}

/* @brief Reads the current value of a numeric sensor.
    * @param ctx Pointer to the PLDM context.
 * @param sensor_id The ID of the numeric sensor to read.
 * @param sensor_data_size Pointer to store the size of the sensor data.
 * @param sensor_operational_state Pointer to store the operational state of the sensor.
 * @param sensor_event_message_enable Pointer to store the event message enable state of the sensor.
 * @param present_reading Pointer to store the present reading of the sensor.
 * @return 0 on success, negative error code on failure.
 */
int get_number_sensor_reading(pldm_ctx_t *ctx, uint16_t sensor_id,
                              uint8_t *sensor_data_size,
                              uint8_t *sensor_operational_state,
                              uint8_t *sensor_event_message_enable,
                              uint8_t *present_state,
                              uint8_t *previous_state,
                              uint8_t *event_state,
                              uint32_t *present_reading)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_get_sensor_reading_req);

    int instance_id = get_instance_id();
    int rc = encode_get_sensor_reading_req(instance_id, sensor_id, 0, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get sensor reading request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[MCTP_MTU_SIZE];
    memset(rbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = MCTP_MTU_SIZE;

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)request, req_len, (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code;

    rc = decode_get_sensor_reading_resp(response, response_len - sizeof(struct pldm_msg_hdr),
                                        &completion_code,
                                        sensor_data_size,
                                        sensor_operational_state,
                                        sensor_event_message_enable,
                                        present_state,
                                        previous_state,
                                        event_state,
                                        (uint8_t *)present_reading);
    if (rc)
    {
        if (rc == PLDM_PLATFORM_INVALID_SENSOR_ID)
        {
            fprintf(stderr, "Invalid sensor id\n");
            rc = -EINVAL;
        }
        else if (rc == PLDM_PLATFORM_REARM_UNAVAILABLE_IN_PRESENT_STATE)
        {
            fprintf(stderr, "Sensor Rearm not in present state\n");
            rc = -ENODEV;
        }
        fprintf(stderr, "Failed to decode get sensor reading response: %d\n", rc);
        return rc;
    }

    if (completion_code != PLDM_SUCCESS)
    {
        // Handle error based on completion code
        switch (completion_code)
        {
        case PLDM_PLATFORM_INVALID_SENSOR_ID:
            fprintf(stderr, "Invalid Sensor ID in request\n");
            return -EINVAL;
        case PLDM_PLATFORM_REARM_UNAVAILABLE_IN_PRESENT_STATE:
            fprintf(stderr, "Sensor reading unavailable\n");
            return -ENODATA;
        default:
            fprintf(stderr, "Error completion code: %d\n", completion_code);
            return -EIO;
        }
    }

    return PLDM_SUCCESS; // Success
}

/* @brief Reads all numeric sensor readings.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */


int get_all_numeric_sensor_readings(void *pctx)
{
    pldm_ctx_t *ctx = (pldm_ctx_t *)pctx;

    if (ctx == NULL || ctx->mctp_ctx == NULL || ctx->platform_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context or Platform Context \n");
        return -EINVAL;
    }

    struct pldm_numeric_sensor_value_pdr *pdr = NULL;
    if (ctx->platform_ctx == NULL || ctx->platform_ctx->numeric_sensor_record_count == 0)
    {
        fprintf(stderr, "No numeric sensors available in platform context\n");
        return -ENODEV;
    }
    //fprintf(stdout, "Reading all numeric sensor readings...\n");

    uint8_t sensor_data_size = 5;
    uint8_t sensor_operational_state = 0;
    uint8_t sensor_event_message_enable = 0;
    uint8_t present_state = 0;
    uint8_t previous_state = 0;
    uint8_t event_state = 0;
    uint32_t present_reading = 0;

    for (int count = 0; count < 7 /*count < ctx->platform_ctx->numeric_sensor_record_count */; count ++)
    {
        pdr = ctx->platform_ctx->numeric_sensor_pdr[count];
        if (pdr == NULL)
        {
            fprintf(stderr, "No more numeric sensor PDR records found\n");
            break;
        }
       
      /*   fprintf(stdout, "Numeric Sensor PDR Record Handle: %u, Sensor ID: %u, Data Size: %u\n",
               pdr->hdr.record_handle, pdr->sensor_id, pdr->sensor_data_size);
  */
        int rc = get_number_sensor_reading(ctx, pdr->sensor_id, &sensor_data_size,
                                           &sensor_operational_state,
                                           &sensor_event_message_enable,
                                           &present_state, &previous_state, &event_state, &present_reading);
        if (rc)
        {
            if (rc == -EINVAL)
                continue;
            return rc;
        } 
            
        /* printf("Sensor ID: %u, Data Size: %u, Operational State: %u, Event Message Enable: %u, "
               "Present State: %u, Previous State: %u, Event State: %u, Present Reading: %u\n",
               pdr->sensor_id, sensor_data_size, sensor_operational_state,
               sensor_event_message_enable, present_state, previous_state,
               event_state, present_reading);
        */
        sensor_data_size = 5;
        sensor_operational_state = 0;
        sensor_event_message_enable = 0;
        present_state = 0;
        previous_state = 0;
        event_state = 0;
        present_reading = 0;
    }

    return PLDM_SUCCESS; // Success
}

/* @brief Sets the operational state of a numeric effecter.
 * @param ctx Pointer to the PLDM context.
 * @param effecter_id The ID of the numeric effecter to set.
 * @param effecter_operational_state The operational state to set for the effecter.
 * @return 0 on success, negative error code on failure.
 */
int set_numeric_effecter_enable(pldm_ctx_t *ctx, uint16_t effecter_id,
                                uint8_t effecter_operational_state)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_set_numeric_effecter_enable_req);

    int instance_id = get_instance_id();
    int rc = encode_set_numeric_effecter_enable_req(
        instance_id, effecter_id,
        effecter_operational_state,
        request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode set numeric effecter enable request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[MCTP_MTU_SIZE];
    memset(rbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = MCTP_MTU_SIZE;

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)request, req_len, (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code = response->payload[0];
    if (completion_code != PLDM_SUCCESS)
    {
        // Handle error based on completion code
        if (completion_code == PLDM_PLATFORM_INVALID_EFFECTER_ID)
        {
            fprintf(stderr, "Invalid Effecter ID in request\n");
            return -EINVAL;
        }
        fprintf(stderr, "Set numeric effecter enable failed with completion code: %d\n", completion_code);
        return -EIO;
    }

    return PLDM_SUCCESS; // Success
}

/* @brief Sets the value of a numeric effecter.
 * @param ctx Pointer to the PLDM context.
 * @param effecter_id The ID of the numeric effecter to set.
 * @param effecter_data_size The size of the effecter data.
 * @param effecter_value Pointer to the value to set for the effecter.
 * @return 0 on success, negative error code on failure.
 */
int set_numeric_effecter_value(pldm_ctx_t *ctx, uint16_t effecter_id,
                               uint8_t effecter_data_size,
                               uint8_t *effecter_value)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    if (effecter_data_size > PLDM_EFFECTER_DATA_SIZE_SINT32)
    {
        fprintf(stderr, "Invalid effecter data size: %d\n", effecter_data_size);
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_set_numeric_effecter_value_req) + effecter_data_size;

    int instance_id = get_instance_id();

    int rc = encode_set_numeric_effecter_value_req(
        instance_id, effecter_id,
        effecter_data_size, effecter_value, request, req_len);
    if (rc)
    {
        fprintf(stderr, "Failed to encode set numeric effecter value request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[MCTP_MTU_SIZE];
    memset(rbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = MCTP_MTU_SIZE;

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)request, req_len, (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code;

    rc = decode_set_numeric_effecter_value_resp(response, response_len - sizeof(struct pldm_msg_hdr), &completion_code);
    if (rc)
    {
        fprintf(stderr, "Failed to decode set numeric effecter value response: %d\n", rc);
        return rc;
    }

    if (completion_code != PLDM_SUCCESS)
    {
        if (completion_code == PLDM_PLATFORM_INVALID_EFFECTER_ID)
        { // Handle error based on completion code
            fprintf(stderr, "Invalid Effecter ID in request\n");
            return -EINVAL;
        }
        fprintf(stderr, "Error completion code: %d\n", completion_code);
        return -EIO;
    }
    return PLDM_SUCCESS; // Success
}
/* @brief Gets the current value of a numeric effecter.
 * @param ctx Pointer to the PLDM context.
 * @param effecter_id The ID of the numeric effecter to get.
 * @param effecter_data_size Pointer to store the size of the effecter data.
 * @param effecter_operational_state Pointer to store the operational state of the effecter.
 * @param pending_value Pointer to store the pending value of the effecter.
 * @param present_value Pointer to store the present value of the effecter.
 * @return 0 on success, negative error code on failure.
 */
int get_number_effecter_value(pldm_ctx_t *ctx, uint16_t effecter_id,
                              uint8_t *effecter_data_size,
                              uint8_t *effecter_operational_state,
                              uint8_t *pending_value,
                              uint8_t *present_value)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_get_numeric_effecter_value_req);

    int instance_id = get_instance_id();
    int rc = encode_get_numeric_effecter_value_req(instance_id, effecter_id, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get numeric effecter value request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[MCTP_MTU_SIZE];
    memset(rbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = MCTP_MTU_SIZE;

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)request, req_len, (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    uint8_t completion_code;

    rc = decode_get_numeric_effecter_value_resp(response, response_len - sizeof(struct pldm_msg_hdr),
                                                &completion_code,
                                                effecter_data_size,
                                                effecter_operational_state,
                                                pending_value,
                                                present_value);
    if (rc)
    {
        fprintf(stderr, "Failed to decode get numeric effecter value response: %d\n", rc);
        return rc;
    }

    if (completion_code != PLDM_SUCCESS)
    {
        // Handle error based on completion code
        if (completion_code == PLDM_PLATFORM_INVALID_EFFECTER_ID)
        {
            fprintf(stderr, "Invalid Effecter ID in request\n");
            return -EINVAL;
        }
        fprintf(stderr, "Get numeric effecter value failed with completion code: %d\n", completion_code);
        return -EIO;
    }
    return PLDM_SUCCESS; // Success
}
