/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>

#ifndef container_of
#define container_of(ptr, type, member) (type *)((char *)(ptr) - (char *)&((type *)0)->member)
#endif

#define binding_to_smbus(b) container_of(b, struct mctp_binding_smbus, binding)

#define SMBUS_READ_RETRY_COUNT (3)
#define SMBUS_READ_DELAY (30 * 1000) // 30ms
				     
#define MCTP_COMMAND_CODE      0x0F
#define MCTP_TARGET_ADDR_INDEX 0
#define DEFAULT_TARGET_ADDRESS 0x40

#define SMBUS_COMMAND_CODE_SIZE  1
#define SMBUS_LENGTH_FIELD_SIZE  1
#define SMBUS_ADDR_OFFSET_TARGET 0x1000

struct mctp_smbus_header_tx {
	uint8_t command_code;
	uint8_t byte_count;
	uint8_t source_target_address;
};

struct mctp_smbus_header_rx {
	uint8_t destination_target_address;
	uint8_t command_code;
	uint8_t byte_count;
	uint8_t source_target_address;
};
int mctp_smbus_read(int fd, uint8_t *buf, uint16_t *length)
{
	size_t len = *length;

	if (buf == NULL || len <= 0) {
		fprintf(stderr, "Parameters are not valid\n");
		return -1;
	}

	len = read(fd, buf, *length);

	if (len < 0 || buf[0] != 0x0F || buf[3] != 0x01) {
		return -1;
	}

	*length = buf[1] + 2;

	return 0;
}

int mctp_smbus_write(int fd, uint8_t *buf, uint16_t len)
{
	len = write(fd, buf, len);

	if (len < 0) {
		fprintf(stderr, "Failed to write\n");
		return -1;
	}
	return 0;
}

int setup_i2c(const char *filename, const uint8_t i2c_addr)
{
	int fd;
	int rv;

	if ((fd = open(filename, O_RDWR)) < 0) {
		fprintf(stderr, "Failed to open the i2c bus. Error code: %d\n", fd);
		return fd;
	}

	if ((rv = ioctl(fd, I2C_SLAVE, i2c_addr)) < 0) {
		fprintf(stderr,
			"Failed to acquire bus access and/or talk to slave. Error code: %d\n", rv);
		return rv;
	}

	return fd;
}
