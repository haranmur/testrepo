#ifndef __MCTP_H__
#define __MCTP_H__

#include <stdint.h>

#define MCTP_MTU_SIZE (72) // 64 + 8 (MCTP header size)
#define MCTP_AMC_I2C_ADDR 0x40
#define MCTP_REQ_DELAY    (40 * 1000) // 20 ms delay for MCTP request

// MCTP FLAGS
#define START_OF_MESSAGE (1 << 7)                  // 0b10000000
#define END_OF_MESSAGE   (1 << 6)                  // 0b01000000
#define PACKET_SEQUENCE  ((1 << 5) | (1 << 4))     // 0b00110000
#define TAG_OWNER        (1 << 3)                  // 0b00001000
#define MESSAGE_TAG_MASK ((1 << 2) | (1 << 1) | 1) // 0b00000111

struct mctp_version {
	uint8_t major;
	uint8_t minor;
	uint8_t upd;
	uint8_t alpha;
};

#define MCTP_UUID_SIZE 16

typedef struct {
	uint8_t slave_address;
	uint8_t src_eid;
	uint8_t dest_eid;
	uint8_t ep_type;
	uint8_t uuid[MCTP_UUID_SIZE];
	struct mctp_version dest_ver;
	uint8_t msg_type;
} mctp_info_t;

typedef struct {
	int fd_i2c;
	mctp_info_t mctp_info;
} mctp_device_info_t;

int mctp_tx(void *ctx, uint8_t tid, uint8_t *payload, uint16_t payload_len);
int mctp_rx(void *ctx, uint8_t tid, uint8_t *response, uint16_t *response_len);
int mctp_init(const char *i2c_filename, const uint8_t i2c_addr, void **ctx);
void mctp_info_print(mctp_device_info_t *mctp_dev);

#endif //__MCTP_H__
