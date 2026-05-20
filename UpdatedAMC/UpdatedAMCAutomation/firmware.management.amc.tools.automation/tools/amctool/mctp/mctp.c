#include "mctp_internal.h"
#include "mctp_control.h"
#include "smbus.h"
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "mctp.h"
#include "../pldm/utils.h"

mctp_device_info_t *mctp_device_info;

extern void buf_print(char *header, uint8_t *data, uint8_t len);

void mctp_info_print(mctp_device_info_t *mctp_dev)
{
	mctp_info_t *info = &mctp_dev->mctp_info;

	fprintf(stdout, "\n--------MCTP INFO--------\n");
	fprintf(stdout, "SRC EID  - 0x%x\n", info->src_eid);
	fprintf(stdout, "DST EID  - 0x%x\n", info->dest_eid);
	fprintf(stdout, "EP TYPE  - 0x%x\n", info->ep_type);
	fprintf(stdout, "MSG TYPE - 0x%x\n", info->msg_type);
	fprintf(stdout, "SLAVE ADDR - 0x%x\n", info->slave_address);
	fprintf(stdout, "Ver Major - 0x%x\n", info->dest_ver.major);
	fprintf(stdout, "Ver Minor - 0x%x\n", info->dest_ver.minor);
	fprintf(stdout, "Ver UPD - 0x%x\n", info->dest_ver.upd);
	fprintf(stdout, "Ver Alpha - 0x%x\n", info->dest_ver.alpha);
	buf_print("UUID", (uint8_t *)info->uuid, MCTP_UUID_SIZE);

	fprintf(stdout, "\n");
}

static uint8_t crc8_calculate(uint16_t d)
{
        const uint32_t poly_check = 0x1070 << 3;
        int i;

        for (i = 0; i < 8; i++)
        {
                if (d & 0x8000)
                {
                        d = d ^ poly_check;
                }
                d = d << 1;
        }

        return (uint8_t)(d >> 8);
}

/* Incremental CRC8 over count bytes in the array pointed to by p */
static uint8_t pec_calculate(uint8_t crc, uint8_t *p, size_t count)
{
        int i;

        for (i = 0; i < count; i++)
        {
                crc = crc8_calculate((crc ^ p[i]) << 8);
        }
        return crc;
}

static uint8_t calculate_pec_byte(uint8_t *buf, size_t len, uint8_t address)
{
	uint8_t pec = pec_calculate(0, &address, 1);
	pec = pec_calculate(pec, buf, len);

	return pec;
}

int mctp_fill_hdr(mctp_request_t *req, mctp_hdr_t *hdr, uint8_t slave_addr)
{
	assert(req != NULL);

	req->i2c_hdr.command_code = MCTP_COMMAND_CODE;
	req->i2c_hdr.bytecount = MCTP_HEADER_SIZE;
	req->i2c_hdr.source_slave_address = (slave_addr << 1) | 0x1;

	req->mctp_hdr.hdr_version = 0x01;
	req->mctp_hdr.destination_eid = hdr->destination_eid;
	req->mctp_hdr.source_eid = hdr->source_eid;
	req->mctp_hdr.flags_seq_tag = 0xc8; // hdr->flags_seq_tag;

	return 0;
}

int mctp_tx(void *ctx, uint8_t tid, uint8_t *payload, uint16_t payload_len)
{
	mctp_device_info_t *dev_ctx = ctx;

	mctp_info_t *mctp_info = &dev_ctx->mctp_info;

	assert(ctx != NULL);

	if (payload == NULL) {
		return -EINVAL;
	}

	if (payload_len > 512) {
		return -EINVAL;
	}

	mctp_hdr_t hdr;
	memset(&hdr, 0, sizeof(hdr));

	unsigned char buf[payload_len];
	i2c_slave_buffer_t *slave_buffer = (i2c_slave_buffer_t *)buf;
	memset(&buf, 0, sizeof(buf));
	uint8_t bytecount;

	slave_buffer->slave_address = mctp_info->slave_address;
	hdr.destination_eid = mctp_info->dest_eid;
	hdr.source_eid = mctp_info->src_eid;
	hdr.flags_seq_tag = 0;

	mctp_fill_hdr(&slave_buffer->mctp_req, &hdr, slave_buffer->slave_address);
	bytecount = slave_buffer->mctp_req.i2c_hdr.bytecount;

	uint8_t *pldm_message_type = slave_buffer->mctp_req.data;
	*pldm_message_type++ = 0x01;
	bytecount++;

	memcpy(pldm_message_type, payload, payload_len);
	bytecount += payload_len;

	slave_buffer->mctp_req.i2c_hdr.bytecount = bytecount;

	uint8_t pec = calculate_pec_byte((uint8_t *)&slave_buffer->mctp_req, bytecount, slave_buffer->slave_address);
	slave_buffer->mctp_req.data[payload_len+1] = pec;
	//fprintf(stdout, "pec byte - 0x%x\n", pec);
	//buf_print("MCTP REQ", slave_buffer->data, bytecount + 3);

	int rc = mctp_smbus_write(dev_ctx->fd_i2c, (uint8_t *)slave_buffer->data, bytecount + 3);
	return rc;
}

uint8_t get_pkt_seq_number(uint8_t seq_number)
{
	uint8_t pkt_seq_num = 0;
	pkt_seq_num = seq_number & PACKET_SEQUENCE;
	return (pkt_seq_num >> 4);
}

int mctp_rx(void *ctx, uint8_t tid, uint8_t *response, uint16_t *response_len)
{
        uint16_t i2c_data_length;

        mctp_device_info_t *dev_ctx = ctx;
        assert(ctx != NULL);
        int retries = 5;

        if (response == NULL || response_len == NULL) {
            return -EINVAL;
        }

        if (*response_len > 1024) {
			fprintf(stderr, "Response length exceeds maximum size - %d\n", *response_len);
			return -EINVAL;
        }

		unsigned char buf[1024];
		uint8_t *rbuf = response;
		uint16_t resp_len = *response_len;

		bool start_of_message = false;

		while (true)
		{
			i2c_data_length = 72; // MCTP_MIN_PACKET_SIZE + MCTP_HEADER_SIZE + 3;
			int i;
			for (i = 0; i < retries; i++)
			{
				usleep(60 * 1000);
				mctp_smbus_read(dev_ctx->fd_i2c, buf, &i2c_data_length);
				if (buf[0] == 0xf)
					break;
			}
			if (i == retries)
			{
				fprintf(stderr, "Failed to read from I2C\n");
				return -EIO;
			}

			int bytecount = buf[1];

			bool som = (buf[6] & START_OF_MESSAGE) == START_OF_MESSAGE ? true : false;
			bool eom = (buf[6] & END_OF_MESSAGE) == END_OF_MESSAGE ? true : false;

			//uint8_t seq = get_pkt_seq_number(buf[6]);
			//fprintf(stdout, "seq - 0x%x\n", seq);

			if (som && eom)
			{
				//fprintf(stdout, " Single Packet : byte count - %d\n", bytecount);
				memcpy(response, buf + 8, bytecount - 6); // Fix it.
				*response_len = bytecount - 6;
				//buf_print("MCTP RSP", buf, bytecount + 3);
				return 0;
			}

			if (som)
			{
				//fprintf(stdout, "MCTP RSP start packet : byte count - %d\n", bytecount);
				memcpy(response, buf + 8, bytecount - 6); // Fix it.
				rbuf = response + 63;
				*response_len = 63;
				if (start_of_message)
				{ // Second Time
					//buf_print("MCTP RSP duplicate", buf, bytecount + 3);
				}
				else
				{ // First Time
					//buf_print("MCTP RSP", buf, bytecount + 3);
					resp_len -=  63;
					start_of_message = true;
				}
			}
			else if (!som && !eom)
			{
				//buf_print("MCTP RSP middle Packet", buf, bytecount + 3);
				memcpy(rbuf, buf + 7, bytecount - 5); // Fix it.
				rbuf += 64;
				*response_len += 64;
				resp_len -= 64;
			}
			else if (eom)
			{
				//buf_print("MCTP RSP last packet", buf, bytecount + 3);
				memcpy(rbuf, buf + 7, bytecount - 5); // Fix it.
				*response_len += bytecount - 5;

				//fprintf(stdout, "%s: response_len - %d\n", __func__, *response_len);
				return 0;
			}
		}

		return -EIO;
}

int mctp_get_version(mctp_info_t *mctp_info)
{
	if (mctp_info == NULL) {
		return -EINVAL;
	}

	unsigned char buf[256] = { 0 };
	i2c_slave_buffer_t *slave_buffer = (i2c_slave_buffer_t *)buf;
	memset(buf, 0, sizeof(buf));

	mctp_hdr_t hdr;
	memset(&hdr, 0, sizeof(hdr));

	hdr.destination_eid = mctp_info->dest_eid;
	hdr.source_eid = mctp_info->src_eid;

	enc_mc_get_version_req(&slave_buffer->mctp_req, 256, NULL, 0, &hdr, mctp_info->slave_address);

	mctp_smbus_write(mctp_device_info->fd_i2c, (uint8_t *)slave_buffer->data,
			 slave_buffer->mctp_req.i2c_hdr.bytecount + 2);

	//buf_print("get version req", slave_buffer->data, slave_buffer->mctp_req.i2c_hdr.bytecount + 2);

	uint16_t i2c_data_length = 0x10;

	mc_get_version_resp_t ver_resp;
	memset(&ver_resp, 0, sizeof(ver_resp));

	for (int i = 0; i < 3; i++) {
		usleep(MCTP_REQ_DELAY);
		mctp_smbus_read(mctp_device_info->fd_i2c, buf, &i2c_data_length);
		if (buf[0] == 0xf) {
			parse_mc_get_version_resp(buf, i2c_data_length, &ver_resp);
			if (ver_resp.completion_code) {
				fprintf(stderr, "Get Version failed with completion code: 0x%x\n", ver_resp.completion_code);
				return -EIO;
			}
			mctp_info->dest_ver.major = ver_resp.major_ver;
			mctp_info->dest_ver.minor = ver_resp.minor_ver;
			mctp_info->dest_ver.upd = ver_resp.upd_ver;
			mctp_info->dest_ver.alpha = ver_resp.alpha;
			//buf_print("get version resp", buf, i2c_data_length);
			return 0; 
		}
	}

	return -EIO;
}

int mctp_get_eid(mctp_info_t *mctp_info)
{
	if (mctp_info == NULL) {
		return -EINVAL;
	}

	unsigned char buf[256] = { 0 };
	memset(buf, 0, sizeof(buf));
	i2c_slave_buffer_t *slave_buffer = (i2c_slave_buffer_t *)buf;

	mctp_hdr_t hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.destination_eid = mctp_info->dest_eid;
	hdr.source_eid = mctp_info->src_eid;
	
	hdr.flags_seq_tag = 1;
	enc_mc_get_eid_req(&slave_buffer->mctp_req, 256, NULL, 0, &hdr, mctp_info->slave_address);

	mctp_smbus_write(mctp_device_info->fd_i2c, (uint8_t *)slave_buffer->data,
			 slave_buffer->mctp_req.i2c_hdr.bytecount + 2);
	
	//buf_print("get eid req", slave_buffer->data, slave_buffer->mctp_req.i2c_hdr.bytecount + 2);

	uint16_t i2c_data_length = 0x0f;	
	mc_get_eid_resp_t eid_resp;
	memset(&eid_resp, 0, sizeof(eid_resp));

	for (int i = 0; i < 3; i++) {
		usleep(MCTP_REQ_DELAY);
		mctp_smbus_read(mctp_device_info->fd_i2c, buf, &i2c_data_length);
		if (buf[0] == 0xf) {
			parse_mc_get_eid_resp(buf, i2c_data_length, &eid_resp);
			if (eid_resp.completion_code) {
				fprintf(stderr, "Get EID failed with completion code: 0x%x\n", eid_resp.completion_code);
				return -EIO;
			}
			
			mctp_info->dest_eid = eid_resp.eid;
			mctp_info->ep_type = eid_resp.ep_type;		
			//buf_print("get get eid resp", buf, i2c_data_length);
			return 0;
		}
	}
	return -EIO;
}

int mctp_get_uuid(mctp_info_t *mctp_info)
{
	if (mctp_info == NULL) {
		return -EINVAL;
	}

	unsigned char buf[256] = { 0 };
	memset(buf, 0, sizeof(buf));
	i2c_slave_buffer_t *slave_buffer = (i2c_slave_buffer_t *)buf;

	mctp_hdr_t hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.destination_eid = mctp_info->dest_eid;
	hdr.source_eid = mctp_info->src_eid;

	enc_mc_get_uuid_req(&slave_buffer->mctp_req, 256, NULL, 0, &hdr, mctp_info->slave_address);

	mctp_smbus_write(mctp_device_info->fd_i2c, (uint8_t *)slave_buffer->data,
			 slave_buffer->mctp_req.i2c_hdr.bytecount + 2);

	//buf_print("get uuid req", slave_buffer->data, slave_buffer->mctp_req.i2c_hdr.bytecount + 2);

	uint16_t i2c_data_length = 0x1B;
	
	mc_get_uuid_resp_t uuid_resp;
	memset(&uuid_resp, 0, sizeof(uuid_resp));

	for (int i = 0; i < 3; i++) {
		usleep(MCTP_REQ_DELAY);
		mctp_smbus_read(mctp_device_info->fd_i2c, buf, &i2c_data_length);
		if (buf[0] == 0xf) {
			parse_mc_get_uuid_resp(buf, i2c_data_length, &uuid_resp);
			if (uuid_resp.completion_code) {
				fprintf(stderr, "Get UUID failed with completion code: 0x%x\n", uuid_resp.completion_code);
				return -EIO;
			}
			memcpy(mctp_info->uuid, uuid_resp.uuid, MCTP_UUID_SIZE);
			//buf_print("get get uuid resp", buf, i2c_data_length);
			return 0;
		}
	}
	return -EIO;
}

int mctp_set_eid(mctp_info_t *mctp_info, uint8_t eid)
{
		if (mctp_info == NULL) {
		return -EINVAL;
	}

	unsigned char buf[256] = { 0 };
	memset(buf, 0, sizeof(buf));
	i2c_slave_buffer_t *slave_buffer = (i2c_slave_buffer_t *)buf;

	mctp_hdr_t hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.destination_eid = mctp_info->dest_eid;
	hdr.source_eid = mctp_info->src_eid;

	enc_mc_set_eid_req(&slave_buffer->mctp_req, 256, NULL, 0, &hdr, mctp_info->slave_address, eid);

	mctp_smbus_write(mctp_device_info->fd_i2c, (uint8_t *)slave_buffer->data,
			 slave_buffer->mctp_req.i2c_hdr.bytecount + 2);

	//buf_print("set eid req", slave_buffer->data, slave_buffer->mctp_req.i2c_hdr.bytecount + 2);

	uint16_t i2c_data_length = 0x0F;

	mc_set_eid_resp_t seid_resp;
	memset(&seid_resp, 0, sizeof(seid_resp));

	for (int i = 0; i < 3; i++) {
		usleep(MCTP_REQ_DELAY);
		mctp_smbus_read(mctp_device_info->fd_i2c, buf, &i2c_data_length);
		if (buf[0] == 0xf) {
			parse_mc_set_eid_resp(buf, i2c_data_length, &seid_resp);
			if (seid_resp.completion_code) {
				fprintf(stderr, "Set EID failed with completion code: 0x%x\n", seid_resp.completion_code);
				return -EIO;
			}
			if (seid_resp.eid != eid) {
				fprintf(stderr, "Set EID failed. Expected: 0x%x, Got: 0x%x\n", eid, seid_resp.eid);
				return -EIO;
			}
			mctp_info->dest_eid = seid_resp.eid;
			//buf_print("get set eid resp", buf, i2c_data_length);
			return 0;
		}
	}
	return -EIO;
}

int mctp_get_msg_type(mctp_info_t *mctp_info)
{
	if (mctp_info == NULL) {
		return -EINVAL;
	}

	unsigned char buf[256] = { 0 };
	memset(buf, 0, sizeof(buf));
	i2c_slave_buffer_t *slave_buffer = (i2c_slave_buffer_t *)buf;

	mctp_hdr_t hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.destination_eid = mctp_info->dest_eid;
	hdr.source_eid = mctp_info->src_eid;
	memset(buf, 0, 256);
	enc_mc_get_msg_type_req(&slave_buffer->mctp_req, 256, NULL, 0, &hdr, mctp_info->slave_address);

	mctp_smbus_write(mctp_device_info->fd_i2c, (uint8_t *)slave_buffer->data,
			 slave_buffer->mctp_req.i2c_hdr.bytecount + 2);
	
	//buf_print("get msg type req", slave_buffer->data, slave_buffer->mctp_req.i2c_hdr.bytecount + 2);
	
	uint16_t i2c_data_length = 0x0e;

	mc_get_msg_type_resp_t msg_type_resp;
	memset(&msg_type_resp, 0, sizeof(msg_type_resp));

	for (int i = 0; i < 3; i++) {
		usleep(MCTP_REQ_DELAY);
		mctp_smbus_read(mctp_device_info->fd_i2c, buf, &i2c_data_length);
		if (buf[0] == 0xf) {
			parse_mc_msg_type_resp(buf, i2c_data_length, &msg_type_resp);
			if (msg_type_resp.completion_code) {
				fprintf(stderr, "Get Message Type failed with completion code: 0x%x\n", msg_type_resp.completion_code);
				return -EIO;
			}
			mctp_info->msg_type = msg_type_resp.type;
			//buf_print("get msg type resp", buf, i2c_data_length);
			return 0;
		}
	}
	return -EIO;
}

int mctp_init(const char *i2c_filename, const uint8_t i2c_addr, void **mctp_ctx)
{
	static bool mctp_initialized = false;
	static bool init_in_progress = false;
	mctp_info_t *mctp_info;

	if (mctp_initialized == true) {
		return -EINVAL;
	}

	if (init_in_progress) {
		return -EBUSY;
	}

	init_in_progress = true;

	mctp_device_info = malloc(sizeof(mctp_device_info_t));
	assert(mctp_device_info != NULL);

	memset((uint8_t*)mctp_device_info, 0, sizeof(mctp_device_info_t));

	mctp_info = &mctp_device_info->mctp_info;
	
	mctp_info->dest_eid = MCTP_AMC_INB_EID;
	mctp_info->src_eid = MCTP_CPU_INB_EID;
	mctp_info->slave_address = MCTP_AMC_I2C_ADDR;

	int rc = setup_i2c(i2c_filename, MCTP_AMC_I2C_ADDR);
	if (rc < 0) {
		fprintf(stderr, "Set UP I2C Bus Error. Exit now!\n");
		return -EEXIST;
	} 

	mctp_device_info->fd_i2c = rc;

	rc = mctp_get_version(mctp_info);
	if (rc) {
		fprintf(stderr, "Get MCTP Version Error. Exit now!\n");
		return -EEXIST;
	}

	rc = mctp_get_uuid(mctp_info);
	if (rc) {
		fprintf(stderr, "Get MCTP UUID Error. Exit now!\n");
		return -EEXIST;
	}

	rc = mctp_get_eid(mctp_info);
	if (rc) {
		fprintf(stderr, "Get MCTP EID Error. Exit now!\n");
		return -EEXIST;
	}
	if (mctp_info->dest_eid == 0) {
		fprintf(stderr, "MCTP EID is not set. Set MCTP EID now!\n");
		mctp_info->dest_eid = MCTP_AMC_INB_EID;
		rc = mctp_set_eid(mctp_info, mctp_info->dest_eid);
		if (rc) {
			fprintf(stderr, "Set MCTP EID Error. Exit now!\n");
			return -EEXIST;
		}
	}

	rc = mctp_get_msg_type(mctp_info);
	if (rc) {
		fprintf(stderr, "Get MCTP Message Type Error. Exit now!\n");
		return -EEXIST;
	}

	if (mctp_ctx != NULL) {
		*mctp_ctx = mctp_device_info;
	}

	return 0;
}