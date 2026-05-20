#include "mctp_control.h"
#include "mctp_internal.h"
#include <assert.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include "../pldm/utils.h"

int enc_mc_set_eid_req(mctp_request_t *req, size_t req_len, uint8_t *payload, size_t payload_len,
		       mctp_hdr_t *hdr, uint8_t slave_addr, uint8_t eid)
{
	assert(req != NULL);
	assert(req_len > payload_len + 9);

	mctp_fill_hdr(req, hdr, slave_addr);

	mc_hdr_t *mc = (mc_hdr_t *)req->data;
	mc->message_type = 0x00;
	mc->req = 1;
	mc->dg = 0;
	mc->instance = 4;
	mc->command_code = MCTP_CONTROL_SET_EID;
	req->i2c_hdr.bytecount += sizeof(mc_hdr_t);

	mc_set_eid_t *eid_cmd = (mc_set_eid_t *)req->data;
	eid_cmd->ops = 0;
	eid_cmd->eid = eid;
	req->i2c_hdr.bytecount += sizeof(eid_cmd->ops) + sizeof(eid_cmd->eid);

	return 0;
}

int parse_mc_set_eid_resp(uint8_t *buf, size_t size, mc_set_eid_resp_t *eid_resp)
{
	mctp_response_t *mctp_resp = (mctp_response_t *)buf;

	mc_set_eid_resp_t *vp = (mc_set_eid_resp_t *)mctp_resp->data;

	if (mctp_resp->i2c_hdr.command_code != MCTP_COMMAND_CODE ||
	    mctp_resp->mctp_hdr.hdr_version != MCTP_HEADER_VERSION) {
		return -EINVAL;
	}

	uint8_t bytecount = mctp_resp->i2c_hdr.bytecount;

	if (bytecount > size) {
		return -EINVAL;
	}

	if (vp->completion_code) {
		return -EIO;
	}

	memcpy(eid_resp, mctp_resp->data, sizeof(mc_set_eid_resp_t));

	return 0;
}

int enc_mc_get_eid_req(mctp_request_t *req, size_t req_len, uint8_t *payload, size_t payload_len,
		       mctp_hdr_t *hdr, uint8_t slave_addr)
{
	assert(req != NULL);
	assert(req_len > payload_len + 9);

	mctp_fill_hdr(req, hdr, slave_addr);

	mc_hdr_t *mc = (mc_hdr_t *)req->data;
	mc->message_type = 0x00;
	mc->req = 1;
	mc->dg = 0;
	mc->instance = 2;
	mc->command_code = MCTP_CONTROL_GET_EID;
	req->i2c_hdr.bytecount += sizeof(mc_hdr_t);

	return 0;
}

int parse_mc_get_eid_resp(uint8_t *buf, size_t size, mc_get_eid_resp_t *eid_resp)
{

	mctp_response_t *mctp_resp = (mctp_response_t *)buf;
	mc_get_eid_resp_t *rsp = (mc_get_eid_resp_t *)mctp_resp->data;

	if (mctp_resp->i2c_hdr.command_code != MCTP_COMMAND_CODE ||
	    mctp_resp->mctp_hdr.hdr_version != MCTP_HEADER_VERSION) {
		return -EINVAL;
	}

	uint8_t bytecount = mctp_resp->i2c_hdr.bytecount;

	if (bytecount > size) {
		return -EINVAL;
	}

	if (rsp->completion_code) {
		return -EIO;
	}

	memcpy(eid_resp, mctp_resp->data, sizeof(mc_get_eid_resp_t));

	return 0;
}

int enc_mc_get_uuid_req(mctp_request_t *req, size_t req_len, uint8_t *payload, size_t payload_len,
			mctp_hdr_t *hdr, uint8_t slave_addr)
{
	assert(req != NULL);
	assert(req_len > payload_len + 9);

	mctp_fill_hdr(req, hdr, slave_addr);

	mc_hdr_t *mc = (mc_hdr_t *)req->data;
	mc->message_type = 0x00;
	mc->req = 1;
	mc->dg = 0;
	mc->instance = 3;
	mc->command_code = MCTP_CONTROL_GET_UUID;
	req->i2c_hdr.bytecount += sizeof(mc_hdr_t);

	return 0;
}

int parse_mc_get_uuid_resp(uint8_t *buf, size_t size, mc_get_uuid_resp_t *uuid_resp)
{
	mctp_response_t *mctp_resp = (mctp_response_t *)buf;

	mc_get_uuid_resp_t *vp = (mc_get_uuid_resp_t *)mctp_resp->data;

	if (mctp_resp->i2c_hdr.command_code != MCTP_COMMAND_CODE ||
	    mctp_resp->mctp_hdr.hdr_version != MCTP_HEADER_VERSION) {
		return -EINVAL;
	}

	uint8_t bytecount = mctp_resp->i2c_hdr.bytecount;

	if (bytecount > size) {
		return -EINVAL;
	}

	if (vp->completion_code) {
		return -EIO;
	}

	memcpy(uuid_resp, mctp_resp->data, sizeof(mc_get_uuid_resp_t));

	return 0;
}

int enc_mc_get_version_req(mctp_request_t *req, size_t req_len, uint8_t *payload,
			   size_t payload_len, mctp_hdr_t *hdr, uint8_t slave_addr)
{
	assert(req != NULL);
	assert(req_len > payload_len + 9);

	mctp_fill_hdr(req, hdr, slave_addr);

	mc_hdr_t *mc = (mc_hdr_t *)req->data;
	mc->message_type = 0x00;
	mc->req = 1;
	mc->dg = 0;
	mc->instance = 1;
	mc->command_code = MCTP_CONTROL_GET_VERSION;
	req->i2c_hdr.bytecount += sizeof(mc_hdr_t);

	mc_get_version_t *ver = (mc_get_version_t *)req->data;
	ver->message_type_number = 0;
	req->i2c_hdr.bytecount += sizeof(ver->message_type_number);

	return 0;
}

int parse_mc_get_version_resp(uint8_t *buf, size_t size, mc_get_version_resp_t *ver_resp)
{
	mctp_response_t *mctp_resp = (mctp_response_t *)buf;
	mc_get_version_resp_t *vp = (mc_get_version_resp_t *)mctp_resp->data;

	if (mctp_resp->i2c_hdr.command_code != MCTP_COMMAND_CODE ||
	    mctp_resp->mctp_hdr.hdr_version != MCTP_HEADER_VERSION) {
		return -EINVAL;
	}

	uint8_t bytecount = mctp_resp->i2c_hdr.bytecount;

	if (bytecount > size) {
		return -EINVAL;
	}

	if (vp->completion_code) {
		return -EIO;
	}

	if (vp->version_count != 1) {
		return -EINVAL;
	}

	memcpy(ver_resp, mctp_resp->data, sizeof(mc_get_version_resp_t));

	return 0;
}

int enc_mc_get_msg_type_req(mctp_request_t *req, size_t req_len, uint8_t *payload,
			    size_t payload_len, mctp_hdr_t *hdr, uint8_t slave_addr)
{
	assert(req != NULL);
	assert(req_len > payload_len + 9);

	mctp_fill_hdr(req, hdr, slave_addr);

	mc_hdr_t *mc = (mc_hdr_t *)req->data;
	mc->message_type = 0x00;
	mc->req = 1;
	mc->dg = 0;
	mc->instance = 5;
	mc->command_code = MCTP_CONTROL_GET_MSG_TYPE;
	req->i2c_hdr.bytecount += sizeof(mc_hdr_t);

	return 0;
}

int parse_mc_msg_type_resp(uint8_t *buf, size_t size, mc_get_msg_type_resp_t *msg_type_resp)
{
	mctp_response_t *mctp_resp = (mctp_response_t *)buf;

	mc_get_msg_type_resp_t *vp = (mc_get_msg_type_resp_t *)mctp_resp->data;

	if (mctp_resp->i2c_hdr.command_code != MCTP_COMMAND_CODE ||
	    mctp_resp->mctp_hdr.hdr_version != MCTP_HEADER_VERSION) {
		return -EINVAL;
	}

	uint8_t bytecount = mctp_resp->i2c_hdr.bytecount;

	if (bytecount > size) {
		return -EINVAL;
	}

	if (vp->completion_code) {
		return -EIO;
	}

	if (vp->count <= 0) {
		return -EINVAL;
	}

	memcpy((uint8_t *)msg_type_resp, mctp_resp->data, sizeof(mc_get_msg_type_resp_t));

	return 0;
}
