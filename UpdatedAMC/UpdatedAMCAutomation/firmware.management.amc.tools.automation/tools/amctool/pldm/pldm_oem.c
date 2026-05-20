#include <endian.h>
#include <stdbool.h>
#include <string.h>

#include "../pldm/pldm.h"
#include "pldm_oem.h"

int encode_pldm_oem_vr_sync_pause_req(uint8_t instance_id, struct pldm_msg *msg)
{
    if (msg == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    struct pldm_header_info header = {0};
    header.instance = instance_id;
    header.msg_type = PLDM_REQUEST;
    header.command = PLDM_OEM_VR_SYNC_PAUSE;
    header.pldm_type = PLDM_OEM_TYPE;
    return pack_pldm_header(&header, &(msg->hdr));
}

int decode_pldm_oem_vr_sync_pause_resp(const struct pldm_msg *msg,
                                 size_t payload_length, uint8_t *cc)
{
    if (msg == NULL || cc == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    if (payload_length != sizeof(uint8_t)) {
        return PLDM_ERROR_INVALID_LENGTH;
    }

     *cc = msg->payload[0];

    return PLDM_SUCCESS;
}

int encode_pldm_oem_vr_sync_resume_req(uint8_t instance_id, struct pldm_msg *msg)
{
    if (msg == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    struct pldm_header_info header = {0};
    header.instance = instance_id;
    header.msg_type = PLDM_REQUEST;
    header.command = PLDM_OEM_VR_SYNC_RESUME;
    header.pldm_type = PLDM_OEM_TYPE;
    return pack_pldm_header(&header, &(msg->hdr));
}

int decode_pldm_oem_vr_sync_resume_resp(const struct pldm_msg *msg,
                                   size_t payload_length, uint8_t *cc)
{
    if (msg == NULL || cc == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    if (payload_length != sizeof(uint8_t)) {
        return PLDM_ERROR_INVALID_LENGTH;
    }

     *cc = msg->payload[0];

    return PLDM_SUCCESS;
}


int encode_pldm_oem_get_svn_req(uint8_t instance_id,
                                    struct pldm_msg *msg)
{
    if (msg == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    struct pldm_header_info header = {0};
    header.instance = instance_id;
    header.msg_type = PLDM_REQUEST;
    header.command = PLDM_OEM_GET_SVN;
    header.pldm_type = PLDM_OEM_TYPE;
    return pack_pldm_header(&header, &(msg->hdr));

}

int decode_pldm_oem_get_svn_resp(const struct pldm_msg *msg,
                                    size_t payload_length,
                                    uint8_t *cc,   struct pldm_oem_svn *svn)
{
    if (msg == NULL || cc == NULL || svn == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    if (payload_length != sizeof(uint8_t) + sizeof(uint32_t)) {
        return PLDM_ERROR_INVALID_LENGTH;
    }

    struct pldm_get_svn_resp *response =
        (struct pldm_get_svn_resp *)msg->payload;

    *cc = response->completion_code;
    *svn = response->svn;

    return PLDM_SUCCESS;
}

int encode_pldm_oem_commit_svn_req(uint8_t instance_id,
                                   struct pldm_msg *msg)
{
    if (msg == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    struct pldm_header_info header = {0};
    header.instance = instance_id;
    header.msg_type = PLDM_REQUEST;
    header.command = PLDM_OEM_COMMIT_SVN;
    header.pldm_type = PLDM_OEM_TYPE;
    int rc = pack_pldm_header(&header, &(msg->hdr));
    if (rc != PLDM_SUCCESS) {
        return rc;
    }

    return PLDM_SUCCESS;
}

int decode_pldm_oem_commit_svn_resp(const struct pldm_msg *msg,
                                    size_t payload_length,
                                    uint8_t *cc)
{
    if (msg == NULL || cc == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    if (payload_length != sizeof(uint8_t)) {
        return PLDM_ERROR_INVALID_LENGTH;
    }

    struct pldm_commit_svn_resp *response =
        (struct pldm_commit_svn_resp *)msg->payload;

    *cc = response->completion_code;

    return PLDM_SUCCESS;
}

int vr_sync_pause(pldm_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context\n");
        return -EINVAL;
    }

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
    int rc = encode_pldm_oem_vr_sync_pause_req(instance_id, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode vr pause request: %d\n", rc);
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

    rc = decode_pldm_oem_vr_sync_pause_resp(&response,
                                             response_len - sizeof(struct pldm_msg_hdr),
                                             &completion_code);
    if (rc)
    {
        fprintf(stderr, "Failed to decode vr pause response: %d\n", rc);
        return rc;
    }

    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Pause operation failed with completion code: %d\n", completion_code);
        return -1;
    }
    
    fprintf(stdout, "PLDM VR sync paused\n");

    return 0;
}

int vr_sync_resume(pldm_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context\n");
        return -EINVAL;
    }

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
    int rc = encode_pldm_oem_vr_sync_resume_req(instance_id, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode vr resume request: %d\n", rc);
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

    rc = decode_pldm_oem_vr_sync_resume_resp(&response,
                                             response_len - sizeof(struct pldm_msg_hdr),
                                             &completion_code);
    if (rc)
    {
        fprintf(stderr, "Failed to decode vr resume response: %d\n", rc);
        return rc;
    }
      
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Resume operation failed with completion code: %d\n", completion_code);
        return -1;
    }

    fprintf(stdout, "PLDM VR sync resumed\n");
	
	return 0;
}


int get_svn(pldm_ctx_t *ctx, struct pldm_oem_svn *svn)
{
    if (ctx == NULL || svn == NULL)
    {
        fprintf(stderr, "Invalid context or SVN pointer\n");
        return -EINVAL;
    }
    fprintf(stderr, "%s\n", __func__);

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t instance_id = get_instance_id();

    struct pldm_msg msg = {0};
    int rc = encode_pldm_oem_get_svn_req(instance_id, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode get SVN request: %d\n", rc);
        return rc;
    }
    
    uint8_t buf[PLDM_MAX_CMDS_PER_TYPE];
    memset(buf, 0, PLDM_MAX_CMDS_PER_TYPE);

    struct pldm_msg *response = (struct pldm_msg *)buf;

    uint16_t req_len = 3;
    uint16_t response_len = sizeof(struct pldm_msg) + 8;
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

    rc = decode_pldm_oem_get_svn_resp(response,
                                      response_len - sizeof(struct pldm_msg_hdr),
                                      &completion_code, svn);
    if (rc)
    {
        fprintf(stderr, "Failed to decode get SVN response: %d\n", rc);
        return rc;
    }
    
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Get SVN operation failed with completion code: %d\n", completion_code);
        return -1;
    }

    fprintf(stdout, "HW SVN : %d, SW SVN : %d\n", svn->hw_svn, svn->sw_svn);

    return 0;
}

int commit_svn(pldm_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        fprintf(stderr, "Invalid PLDM context\n");
        return -EINVAL;
    }
    fprintf(stderr, "%s\n", __func__);

    if (ctx->mctp_ctx == NULL)
    {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }

    uint8_t instance_id = get_instance_id();

    struct pldm_msg msg = {0};
    int rc = encode_pldm_oem_commit_svn_req(instance_id, &msg);
    if (rc)
    {
        fprintf(stderr, "Failed to encode commit SVN request: %d\n", rc);
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

    rc = decode_pldm_oem_commit_svn_resp(&response,
                                         response_len - sizeof(struct pldm_msg_hdr),
                                         &completion_code);
    if (rc)
    {
        fprintf(stderr, "Failed to decode commit SVN response: %d\n", rc);
        return rc;
    }
    
    if (completion_code != PLDM_SUCCESS)
    {
        fprintf(stderr, "Commit SVN operation failed with completion code: %d\n", completion_code);
        return -1;
    }

    fprintf(stdout, "SVN committed successfully\n");

    return 0;
}

int pldm_oem_vr_sync(pldm_ctx_t *ctx) 
{
    if (ctx == NULL) {
        fprintf(stderr, "Invalid PLDM context\n");
        return -EINVAL;
    }

    if (ctx->mctp_ctx == NULL) {
        fprintf(stderr, "Invalid MCTP context\n");
        return -EINVAL;
    }
	int rc = vr_sync_pause(ctx);
	if (rc) {
		fprintf(stderr, "vr_sync_pause failed with error code: %d\n", rc);
		return rc;
	}
	
	rc = vr_sync_resume(ctx);
	if (rc) {
		fprintf(stderr, "vr_sync_resume failed with error code: %d\n", rc);
		return rc;
	}
    return rc;
}

int pldm_oem_svn(pldm_ctx_t *ctx) 
{
    struct pldm_oem_svn svn = {0};

    int rc = get_svn(ctx, &svn);
    if (rc) {
        fprintf(stderr, "get_svn failed with error code: %d\n", rc);
        return rc;
    }

    rc = commit_svn(ctx);
    if (rc) {
        fprintf(stderr, "commit_svn failed with error code: %d\n", rc);
        return rc;
    }

    //ctx->pldm_oem_type.svn = svn;

    return rc;
}

int pldm_oem_init(pldm_ctx_t *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "PLDM is not initialized\n");
		return -EINVAL;
	}

	if (ctx->pldm_oem_type.is_supported == false) {
		fprintf(stderr, "PLDM OEM type is not supported\n");
		return -ENOTSUP;
	}

	return 0;
}