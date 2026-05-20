#ifndef __MCTP_INTERNAL_H__
#define __MCTP_INTERNAL_H__

#include <stdint.h>

/* Definitions for flags_seq_tag field */
#define MCTP_HDR_FLAG_SOM  (1 << 7)
#define MCTP_HDR_FLAG_EOM  (1 << 6)
#define MCTP_HDR_FLAG_TO   (1 << 3)
#define MCTP_HDR_TO_SHIFT  (3)
#define MCTP_HDR_TO_MASK   (1)
#define MCTP_HDR_SEQ_SHIFT (4)
#define MCTP_HDR_SEQ_MASK  (0x3)
#define MCTP_HDR_TAG_SHIFT (0)
#define MCTP_HDR_TAG_MASK  (0x7)

#define MCTP_MESSAGE_TO_SRC           true
#define MCTP_MESSAGE_TO_DST           false
#define MCTP_MESSAGE_CAPTURE_OUTGOING true
#define MCTP_MESSAGE_CAPTURE_INCOMING false

#define MCTP_HEADER_VERSION (0x01)

/* Baseline Transmission Unit and packet size */
#define MCTP_BTU 64

#define I2C_MAX_MSG_SIZE 256

typedef struct __attribute__((packed)) {
	uint8_t command_code;
	uint8_t bytecount;
	uint8_t source_slave_address;
} mctp_i2c_hdr_t;

typedef struct __attribute__((packed)) {
	uint8_t hdr_version;
	uint8_t destination_eid;
	uint8_t source_eid;
	uint8_t flags_seq_tag;
} mctp_hdr_t;

typedef struct __attribute__((packed)) {
	mctp_i2c_hdr_t i2c_hdr;
	mctp_hdr_t mctp_hdr;
	uint8_t data[0];
} mctp_request_t;

typedef struct __attribute__((packed)) {
	mctp_i2c_hdr_t i2c_hdr;
	mctp_hdr_t mctp_hdr;
	uint8_t data[0];
} mctp_response_t;

#define MCTP_COMMAND_CODE         (0x0F)
#define MCTP_HEADER_SIZE          (5)
#define MCTP_MESSAGE_TYPE_CONTROL (0)
#define MCTP_MESSAGE_TYPE_PLDM    (1)

typedef struct __attribute__((packed)) {
	uint8_t slave_address;
	union {
		uint8_t data[I2C_MAX_MSG_SIZE];
		mctp_request_t mctp_req;
	};
	uint16_t size; /* size of used data (not including slave_address) */
} i2c_slave_buffer_t;



extern int mctp_fill_hdr(mctp_request_t *req, mctp_hdr_t *hdr, uint8_t slave_addr);

#endif //__MCTP_INTERNAL_H__
