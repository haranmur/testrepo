#ifndef PLDM_H
#define PLDM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../libpldm/base.h"
#include "../libpldm/firmware_update.h"
#include "fwpkg.h"
#include "../libpldm/platform.h"
#include "pldm_mc.h"

#define PLDM_SUPPORTED_TYPES (8)

#define PLDM_MAX_TYPES	       64
#define PLDM_MAX_CMDS_PER_TYPE 256
#define PLDM_MAX_TIDS	       256
#define PLDM_TID_UNASSIGNED    0x00

/* Message payload lengths */
#define PLDM_GET_COMMANDS_REQ_BYTES 5
#define PLDM_GET_VERSION_REQ_BYTES  6

/* Response lengths are inclusive of completion code */
#define PLDM_GET_TYPES_REQ_BYTES     0
#define PLDM_GET_TYPES_RESP_BYTES    9
#define PLDM_GET_TID_REQ_BYTES	     0
#define PLDM_GET_TID_RESP_BYTES	     2
#define PLDM_SET_TID_REQ_BYTES	     1
#define PLDM_SET_TID_RESP_BYTES	     1
#define PLDM_GET_COMMANDS_RESP_BYTES 33
/* Response data has only one version and does not contain the checksum */
#define PLDM_GET_VERSION_RESP_BYTES		   10
#define PLDM_MULTIPART_RECEIVE_REQ_BYTES	   18
#define PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES 10

#define PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES  10
#define PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES 11

#define PLDM_DESCRIPTOR_SIZE_BYTES  6

typedef struct {
	ver32_t version;
	bool is_supported;
	uint8_t pldm_commands[PLDM_MAX_CMDS_PER_TYPE/8];
}pldm_type_info_t;

typedef struct {
	uint16_t descriptor_type;
	uint16_t descriptor_len;
	uint8_t descriptor_data[PLDM_DESCRIPTOR_SIZE_BYTES];
} pldm_device_identifiers_t;

typedef struct {
	struct component_parameter_table comp_table;
	uint8_t active_comp_ver_str[PLDM_FWU_COMP_VER_STR_SIZE_MAX];
	uint8_t pending_comp_ver_str[PLDM_FWU_COMP_VER_STR_SIZE_MAX];
} firmware_component_info_t;

#define PLDM_COMPONENTS_MAX 4

typedef struct {
	bitfield32_t capabilities_during_update;
	uint16_t comp_count;
	uint8_t active_comp_image_set_ver_str_type;
	uint8_t active_comp_image_set_ver_str_len;
	uint8_t active_comp_image_set_ver_str[PLDM_FWU_COMP_VER_STR_SIZE_MAX];
	uint8_t pending_comp_image_set_ver_str_type;
	uint8_t pending_comp_image_set_ver_str_len;
    uint8_t pending_comp_image_set_ver_str[PLDM_FWU_COMP_VER_STR_SIZE_MAX];
	firmware_component_info_t component_info[PLDM_COMPONENTS_MAX];
} firmware_parameters_info_t;

typedef struct {
    uint8_t current_state;
    uint8_t previous_state;
    uint8_t aux_state;
    uint8_t aux_state_status;
    uint8_t progress_percent;
    uint8_t reason_code;
    bitfield32_t update_option_flags_enabled;
} __attribute__((packed)) pldm_fwu_status_t;

typedef enum
{
	PLDM_FWU_STATE_IDLE,
	PLDM_FWU_STATE_PREPARING,
	PLDM_FWU_STATE_DOWNLOADING,
	PLDM_FWU_STATE_TRANSFER_COMPLETED,
	PLDM_FWU_STATE_VERIFY_COMPLETED,
	PLDM_FWU_STATE_APPLY_COMPLETED,
	PLDM_FWU_STATE_ACTIVATION_COMPLETED,
	PLDM_FWU_STATE_FAILED
} fwu_state_machine_t;

typedef struct pldm_ctx {
	void *mctp_ctx;
	uint8_t tid;
	uint8_t uuid[UUID_SIZE];
	uint8_t pldm_types_count;
	pldm_type_info_t pldm_types[PLDM_SUPPORTED_TYPES];
	pldm_type_info_t pldm_oem_type;
	uint8_t descriptor_count;
	pldm_device_identifiers_t  pldm_device_id;	
	firmware_parameters_info_t firmware_parameters;
	firmware_package_t firmware_package;
	pldm_fwu_status_t fwu_status;
	fwu_state_machine_t state;
	void *fru_ctx;
	pldm_platform_ctx_t *platform_ctx;
} pldm_ctx_t;

int pldm_send_recv_msg_over_mctp(void *ctx, uint8_t tid, uint8_t *request, uint16_t req_len,
				 uint8_t *response, uint16_t *response_len);
int pldm_recv_msg_over_mctp(void *ctx, uint8_t tid, uint8_t *response, uint16_t *response_len);
int pldm_send_msg_over_mctp(void *ctx, uint8_t tid, uint8_t *request, uint16_t req_len);				 
int get_instance_id(void);
void buf_print(char *header, uint8_t *data, uint8_t len);

#endif /* PLDM_H */
