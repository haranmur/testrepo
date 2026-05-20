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

/* * @brief Sets the operational state and event message enable for a state sensor.
 * @param ctx Pointer to the PLDM context.
 * @param sensor_id The ID of the state sensor to set.
 * @param composite_sensor_count The number of composite sensors.
 * @param op_fields Array of state sensor operation fields.
 * @return 0 on success, negative error code on failure.
 */
int set_state_sensor_enables(pldm_ctx_t *ctx, uint16_t sensor_id,
                                 uint8_t composite_sensor_count,
                                 state_sensor_op_field *op_fields)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_set_state_sensor_enable_req) 
                                        + sizeof (state_sensor_op_field) * composite_sensor_count - 1;
    
    int instance_id = get_instance_id();

    int rc = encode_set_state_sensor_enable_req(
        instance_id, sensor_id, composite_sensor_count, op_fields, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode set state sensor enables request: %d\n", rc);
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
        case PLDM_PLATFORM_SET_EFFECTER_UNSUPPORTED_SENSORSTATE:
            fprintf(stderr, "Event Generation is not Supported\n");
            return -EIO;
        default:
            fprintf(stderr, "Error completion code: %d\n", completion_code);
            return -EIO;
        }
    }   
    return PLDM_SUCCESS; // Success
}


/* @brief Gets the readings from a state sensor.
 * @param ctx Pointer to the PLDM context.
 * @param sensor_id The ID of the state sensor to read.
 * @param composite_sensor_count The number of composite sensors.
 * @param field Pointer to an array of get_sensor_state_field structures to hold the readings.
 * @return 0 on success, negative error code on failure.
 */
int get_state_sensor_readings(pldm_ctx_t *ctx, uint16_t sensor_id,
                                 uint8_t *composite_sensor_count,
                                 get_sensor_state_field *field)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_get_state_sensor_readings_req);
    bitfield8_t sensor_rearm = {0}; // Initialize to zero

    int instance_id = get_instance_id();
    int rc = encode_get_state_sensor_readings_req(instance_id, sensor_id, sensor_rearm, 0, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get sensor readings request: %d\n", rc);
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

    //buf_print("Get State Sensor Readings Response", (uint8_t *)response, response_len);

    rc = decode_get_state_sensor_readings_resp(response, response_len - sizeof(struct pldm_msg_hdr), &completion_code,
                                               composite_sensor_count, field);
    if (rc)
    {
        if (rc == PLDM_PLATFORM_INVALID_SENSOR_ID)
        {
            fprintf(stderr, "Invalid sensor id\n");
            rc = -EINVAL;
        }
        fprintf(stderr, "Failed to decode get sensor readings response: %d\n", rc);
        return -EIO;
    }

    return PLDM_SUCCESS; // Success
}
/* @brief Gets the readings from all state sensors.
 * @param pctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 * 
 * This function retrieves readings from all state sensors defined in the platform context.
 * It iterates through each sensor, retrieves its readings, and prints the results.
 * If no state sensors are available, it returns -ENODEV.
*/

int get_all_state_sensors_reading(void *pctx)
{
    pldm_ctx_t *ctx = (pldm_ctx_t *)pctx;

    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }


    uint8_t sensor_count = ctx->platform_ctx->state_sensor_record_count;
    get_sensor_state_field field[8];

    int rc = 0;
    if (sensor_count == 0)
    {
        fprintf(stderr, "No state sensors available\n");
        return -ENODEV; // No devices found
    }
    for (uint16_t i = 0; i < sensor_count; i++)
    {
        uint16_t sensor_id = ctx->platform_ctx->state_sensor_pdr[i]->sensor_id;
        uint8_t composite_sensor_count = 8;
        rc = get_state_sensor_readings(ctx, sensor_id, &composite_sensor_count, field);
        if (rc)
        {
            fprintf(stderr, "Failed to get state sensor readings for sensor ID %d: %d\n", sensor_id, rc);
            return rc;
        }
/*         printf("Sensor ID: %d, Composite Sensor Count: %d\n", sensor_id, composite_sensor_count);
        for (uint8_t j = 0; j < composite_sensor_count; j++)
        {
            printf("Composite Sensor %d: Operational State: %d, Present State: %d, Previous State: %d, Event State: %d\n",
                   j, field[j].sensor_op_state, field[j].present_state,
                   field[j].previous_state, field[j].event_state);
        } 
*/
    }

    return PLDM_SUCCESS; // Success
}

/* @brief Sets the operational state and event message enable for a state effecter.
 * @param ctx Pointer to the PLDM context.
 * @param effecter_id The ID of the state effecter to set.
 * @param composite_effecter_count The number of composite effecters.
 * @param op_fields Array of state effecter operation fields.
 * @return 0 on success, negative error code on failure.
 */
int set_state_effecter_enables(pldm_ctx_t *ctx, uint16_t effecter_id,
    					    const uint8_t composite_effecter_count,
					        state_effecter_op_field *op_fields)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(struct pldm_set_state_effecter_enable_req);

    int instance_id = get_instance_id();
    int rc = encode_set_state_effecter_enable_req(
        instance_id, effecter_id, composite_effecter_count, op_fields, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode set state effecter enable request: %d\n", rc);
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
        fprintf(stderr, "Error completion code: %d\n", completion_code);
        return -EIO;
    }
    return PLDM_SUCCESS; // Success
}
/* @brief Resets the AIC (Application Interface Controller).
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int aic_reset(void *pctx)
{
    pldm_ctx_t *ctx = (pldm_ctx_t *)pctx;

    fprintf(stderr, "AIC Reset\n");

    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    state_effecter_op_field op_field = {0};
    op_field.effecter_operational_state = EFFECTER_OPER_STATE_DISABLED; // Set to disabled state
    op_field.event_message_enable = PLDM_NO_CHANGE_EVENTS;

    int rc = set_state_effecter_enables(ctx, PLDM_STATE_EFFECTER_AIC_RESET,
                                 1,
                                 &op_field);
    if (rc)
    {
        fprintf(stderr, "Failed to set AIC reset state effecter enables: %d\n", rc);
        return rc;
    }
    fprintf(stderr, "AIC Reset State Effecter Enabled\n");
    
    return PLDM_SUCCESS; // Success
}

/* @brief Sets the state of a state effecter.
 * @param ctx Pointer to the PLDM context.
 * @param effecter_id The ID of the state effecter to set.
 * @param composite_effecter_count The number of composite effecters.
 * @param fields Array of set_effecter_state_field structures containing the state to set.
 * @return 0 on success, negative error code on failure.
 */
int set_state_effecter_states(pldm_ctx_t *ctx, uint16_t effecter_id,
                                 uint8_t composite_effecter_count,
                                 set_effecter_state_field *fields)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) + sizeof(effecter_id) + sizeof (composite_effecter_count) +
                                sizeof(set_effecter_state_field) * composite_effecter_count;

    int instance_id = get_instance_id();
    int rc = encode_set_state_effecter_states_req(
        instance_id, effecter_id, composite_effecter_count, fields, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode set state effecter state request: %d\n", rc);
        return rc;
    }

    uint8_t rbuf[MCTP_MTU_SIZE];
    memset(rbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *response = (struct pldm_msg *)rbuf;
    uint16_t response_len = MCTP_MTU_SIZE;

    buf_print("Set State Effecter States Request", (uint8_t *)request, req_len);

    rc = pldm_send_recv_msg_over_mctp(ctx->mctp_ctx, ctx->tid, (uint8_t *)request, req_len, (uint8_t *)response, &response_len);
    if (rc)
    {
        fprintf(stderr, "Failed to send/receive message: %d\n", rc);
        return rc;
    }

    buf_print("Set State Effecter States Response", (uint8_t *)response, response_len);
    uint8_t completion_code;
    rc = decode_set_state_effecter_states_resp(response, response_len - sizeof(struct pldm_msg_hdr), &completion_code);
    if (rc)
    {
        fprintf(stderr, "Failed to decode set state effecter state response: %d\n", rc);
        return rc;
    }

    if (completion_code != PLDM_SUCCESS)
    {
        // Handle error based on completion code
        switch (completion_code)
        {
        case PLDM_PLATFORM_INVALID_EFFECTER_ID:
            fprintf(stderr, "Invalid Effecter ID in request\n");
            return -EINVAL;
        case PLDM_PLATFORM_INVALID_STATE_VALUE:
            fprintf(stderr, "Invalid State\n");
            return -EIO;
        case PLDM_PLATFORM_SET_EFFECTER_UNSUPPORTED_SENSORSTATE:
            fprintf(stderr, "State is not Supported\n");
            return -EIO;
        default:
            fprintf(stderr, "Error completion code: %d\n", completion_code);
            return -EIO;
        }
    }
    return PLDM_SUCCESS; // Success
}

/* @brief Sets the FRU lock state.
 * @param ctx Pointer to the PLDM context.
 * @param lock Boolean indicating whether to lock (true) or unlock (false) the FRU.
 * @return 0 on success, negative error code on failure.
 */
int set_fru_lock_state(void *ctx, bool lock)
{
    pldm_ctx_t *pldm_ctx = (pldm_ctx_t *)ctx;

    if (pldm_ctx == NULL || pldm_ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    set_effecter_state_field field;
    field.set_request = PLDM_REQUEST_SET;
    field.effecter_state = lock ? PLDM_FRU_LOCK : PLDM_FRU_UNLOCK;

    int rc = set_state_effecter_states(pldm_ctx, PLDM_STATE_EFFECTER_OEM_FRU_LOCK,
                                       1, &field);
    if (rc)
    {
        fprintf(stderr, "Failed to set FRU lock state: %d\n", rc);
        return rc;
    }

    printf("FRU Lock State Set to: %s\n", lock ? "Locked" : "Unlocked");
    return PLDM_SUCCESS; // Success
}

/* @brief Gets the states of a state effecter.
 * @param ctx Pointer to the PLDM context.
 * @param effecter_id The ID of the state effecter to query.
 * @param composite_effecter_count Pointer to store the number of composite effecters.
 * @param fields Array to hold the state effecter states.
 * @return 0 on success, negative error code on failure.
 */
int get_state_effecter_states(pldm_ctx_t *ctx, uint16_t effecter_id,
                                 uint8_t *composite_effecter_count,
                                 get_effecter_state_field *fields)
{
    if (ctx == NULL || ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t tbuf[MCTP_MTU_SIZE];
    memset(tbuf, 0, MCTP_MTU_SIZE);
    struct pldm_msg *request = (struct pldm_msg *)tbuf;
    uint16_t req_len = sizeof(struct pldm_msg_hdr) +sizeof (struct pldm_get_state_effecter_states_req);

    int instance_id = get_instance_id();
    int rc = encode_get_state_effecter_states_req(instance_id, effecter_id, request);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get state effecter states request: %d\n", rc);
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

    buf_print("Get State Effecter States Response", (uint8_t *)response, response_len);
    uint8_t completion_code;
    rc = decode_get_state_effecter_states_resp(response, response_len - sizeof(struct pldm_msg_hdr), &completion_code,
                                               composite_effecter_count, fields);
    if (rc)
    {
        if (rc == PLDM_PLATFORM_INVALID_EFFECTER_ID)
        {
            fprintf(stderr, "Invalid effecter id\n");
            rc = -EINVAL;
        }
        fprintf(stderr, "Failed to decode get state effecter states response: %d\n", rc);
        return rc;
    }

    return PLDM_SUCCESS; // Success
}
/* @brief Gets the FRU lock state.
 * @param ctx Pointer to the PLDM context.
 * @return 0 on success, negative error code on failure.
 */
int get_fru_lock_state(void *ctx)
{
    pldm_ctx_t *pldm_ctx = (pldm_ctx_t *)ctx;

    if (pldm_ctx == NULL || pldm_ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context or MCTP context\n");
        return -EINVAL;
    }

    uint8_t composite_effecter_count = 1;
    get_effecter_state_field field[1];

    int rc = get_state_effecter_states(pldm_ctx, PLDM_STATE_EFFECTER_OEM_FRU_LOCK,
                                       &composite_effecter_count, field);
    if (rc)
    {
        fprintf(stderr, "Failed to get AIC states: %d\n", rc);
        return rc;
    }

    printf("AIC States: Operational State: %d, Pending State: %d, Present State: %d\n",
           field[0].effecter_op_state, field[0].pending_state, field[0].present_state);

    return PLDM_SUCCESS; // Success
}