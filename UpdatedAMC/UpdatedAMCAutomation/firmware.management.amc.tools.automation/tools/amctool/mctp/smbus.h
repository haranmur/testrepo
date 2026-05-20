#ifndef __SMBUS_H__
#define __SMBUS_H__

#include <stdint.h>
#include <stdlib.h>

extern int mctp_smbus_read(int fd, uint8_t *buf, uint16_t *len);
extern int mctp_smbus_write(int fd, uint8_t *buf, uint16_t len);
extern int setup_i2c(const char *filename, const uint8_t i2c_addr);

#endif
