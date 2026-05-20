#ifndef __MCTP_CONTROL_H__
#define __MCTP_CONTROL_H__
#include "mctp.h"
#include "mctp_internal.h"
#include <stdint.h>
#include <stdlib.h>

#define MCTP_CONTROL_REQ       (0x80)
#define MCTP_CONTROL_DG        (0x40)
#define MCTP_CONTROL_INST_MASK (0x1F)

#define MCTP_CONTROL_SET_EID      (0x01)
#define MCTP_CONTROL_GET_EID      (0x02)
#define MCTP_CONTROL_GET_UUID     (0x03)
#define MCTP_CONTROL_GET_VERSION  (0x04)
#define MCTP_CONTROL_GET_MSG_TYPE (0x05)

#define MCTP_CPU_INB_EID (0x10)
#define MCTP_AMC_INB_EID (0x11)

typedef struct __attribute__((packed)) {
	uint8_t message_type;
	struct {
		uint8_t req: 1;
		uint8_t dg: 1;
		uint8_t rsrvd: 1;
		uint8_t instance: 5;
	};
	uint8_t command_code;
	uint8_t data[0];
} mc_hdr_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t completion_code;
} mc_resp_hdr_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t message_type_number;
} mc_get_version_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t completion_code;
	uint8_t version_count;
	uint8_t major_ver;
	uint8_t minor_ver;
	uint8_t upd_ver;
	uint8_t alpha;
} mc_get_version_resp_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t ops;
	uint8_t eid;
} mc_set_eid_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t completion_code;
	uint8_t rsvd1: 2;
	uint8_t status: 2;
	uint8_t rsvd2: 2;
	uint8_t alloc_status: 2;
	uint8_t eid;
	uint8_t pool;
} mc_set_eid_resp_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t completion_code;
	uint8_t eid;
	uint8_t ep_type;
	uint8_t medium_info;
} mc_get_eid_resp_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t completion_code;
	uint8_t uuid[MCTP_UUID_SIZE];
} mc_get_uuid_resp_t;

typedef struct __attribute__((packed)) {
	mc_hdr_t hdr;
	uint8_t completion_code;
	uint8_t count;
	uint8_t type;
} mc_get_msg_type_resp_t;

extern int enc_mc_set_eid_req(mctp_request_t *req, size_t req_len, uint8_t *payload,
			      size_t payload_len, mctp_hdr_t *hdr, uint8_t slave_addr, uint8_t eid);

extern int parse_mc_set_eid_resp(uint8_t *buf, size_t size, mc_set_eid_resp_t *eid_resp);

extern int enc_mc_get_eid_req(mctp_request_t *req, size_t req_len, uint8_t *payload,
			      size_t payload_len, mctp_hdr_t *hdr, uint8_t slave_addr);

extern int parse_mc_get_eid_resp(uint8_t *buf, size_t size, mc_get_eid_resp_t *eid_resp);

extern int enc_mc_get_uuid_req(mctp_request_t *req, size_t req_len, uint8_t *payload,
			       size_t payload_len, mctp_hdr_t *hdr, uint8_t slave_addr);

extern int parse_mc_get_uuid_resp(uint8_t *buf, size_t size, mc_get_uuid_resp_t *uuid_resp);

extern int enc_mc_get_version_req(mctp_request_t *req, size_t req_len, uint8_t *payload,
				  size_t payload_len, mctp_hdr_t *hdr, uint8_t slave_addr);

extern int parse_mc_get_version_resp(uint8_t *buf, size_t size, mc_get_version_resp_t *ver_resp);

extern int enc_mc_get_msg_type_req(mctp_request_t *req, size_t req_len, uint8_t *payload,
				   size_t payload_len, mctp_hdr_t *hdr, uint8_t slave_addr);

int parse_mc_msg_type_resp(uint8_t *buf, size_t size, mc_get_msg_type_resp_t *msg_type_resp);

#endif
