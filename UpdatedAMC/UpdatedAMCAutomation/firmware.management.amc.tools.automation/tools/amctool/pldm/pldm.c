#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include "../libpldm/base.h"
#include "config.h"
#include "pldm.h"
#include "../mctp/mctp.h"
#include "pldm_oem.h"
#include "pldm_fru.h"
#include "utils.h"
#include "pldm_mc.h"

typedef enum {
	AMCTOOL_MAJOR_VERSION = 0,
	AMCTOOL_MINOR_VERSION = 8,
	AMCTOOL_UPDATE_VERSION = 0,
	AMCTOOL_ALPHA_VERSION = 0,
}amctool_version_t;

extern int fwupdate(pldm_ctx_t *ctx, const char *pkg_file_path);

// Structure to hold all command line options
typedef struct {
    int verbose_flag;
    int version_flag;
    int interactive_flag;
    char config_file[PATH_MAX];
    
    // Initialization flags
    int mctp_init_flag;
    int pldm_init_flag;
    int pldm_oem_init_flag;
    int pldm_fru_init_flag;
    int pldm_mc_init_flag;
    
    // Operation flags
    int parse_pkg_flag;
    int fwu_flag;
    int pldm_vr_sync_flag;
    int pldm_svn_flag;
    int pldm_sensor_enable_flag;
    int pldm_aic_reset_flag;
    
    // Info flags
    int mctp_info_flag;
    int pldm_info_flag;
    int pkg_info_flag;
    int fru_info_flag;
    int pldm_mc_info_flag;
} cmd_options_t;

// Function declarations
static void init_cmd_options(cmd_options_t *opts);
static void print_version(void);
static int parse_cmd_options(int argc, char **argv, cmd_options_t *opts);
static int validate_cmd_options(const cmd_options_t *opts);
static int execute_operations(const cmd_options_t *opts, const amc_config_t *amc_config);
static int execute_info_commands(const cmd_options_t *opts, const amc_config_t *amc_config);
static int interactive_mode(const amc_config_t *amc_config);
static void print_interactive_help(void);
static int parse_interactive_command(const char *input, cmd_options_t *opts);

extern void printPldmTypes(uint8_t *types, uint8_t length);
extern int pldm_oem_init(pldm_ctx_t *ctx);
extern int pldm_fru_init(pldm_ctx_t *ctx);
extern int pldm_platform_init(pldm_ctx_t *ctx);

static void *mctp_ctx;
static pldm_ctx_t *pldm_ctx;

void pldm_base_print_error(int rc)
{
	enum pldm_completion_codes completion_code = (enum pldm_completion_codes)rc;
	if (rc == PLDM_SUCCESS)
	{
		fprintf(stdout, "Operation completed successfully\n");
		return;
	}
	switch (completion_code)
	{
	case PLDM_ERROR_INVALID_DATA:
		fprintf(stderr, "PLDM base error: Invalid data\n");
		break;
	case PLDM_ERROR_INVALID_LENGTH:
		fprintf(stderr, "PLDM base error: Invalid length\n");
		break;
	case PLDM_ERROR_NOT_READY:
		fprintf(stderr, "PLDM base error: Not ready\n");
		break;
	case PLDM_ERROR_UNSUPPORTED_PLDM_CMD:
		fprintf(stderr, "PLDM base error: Unsupported PLDM command\n");
		break;
	case PLDM_ERROR_INVALID_PLDM_TYPE:
		fprintf(stderr, "PLDM base error: Invalid PLDM type\n");
		break;
	case PLDM_INVALID_TRANSFER_OPERATION_FLAG:
		fprintf(stderr, "PLDM base error: Invalid transfer operation flag\n");
		break;
	case PLDM_ERROR:
		fprintf(stderr, "PLDM base error: General error\n");
		break;
	default:
		fprintf(stderr, "PLDM base error: Unknown error code %d\n", rc);
		break;
	}
}

int get_instance_id(void)
{
	static uint8_t instance_id = 0;

	instance_id++;
	instance_id = instance_id <= PLDM_INSTANCE_MAX ? instance_id : 1;

	return instance_id;
}

int pldm_send_recv_msg_over_mctp(void *ctx, uint8_t tid, uint8_t *request, uint16_t req_len,
								 uint8_t *response, uint16_t *response_len)
{
	if (ctx == NULL || request == NULL || response == NULL || response_len == NULL)
	{
		fprintf(stderr, "Invalid arguments passed to pldm_send_recv_msg_over_mctp\n");
		return -EINVAL;
	}
	int write_rc = mctp_tx(ctx, tid, request, req_len);
	if (write_rc != 0)
	{
		fprintf(stderr, "mctp_tx failed with error code: %d\n", write_rc);
		return write_rc;
	}

	int rc = mctp_rx(ctx, tid, response, response_len);
	if (rc != 0)
	{
		fprintf(stderr, "mctp_rx failed with error code: %d\n", rc);
		return rc;
	}
	return rc;
}

int pldm_recv_msg_over_mctp(void *ctx, uint8_t tid, uint8_t *response, uint16_t *response_len)
{
	int rc = mctp_rx(ctx, tid, response, response_len);
	if (rc != 0)
	{
		fprintf(stderr, "mctp_rx failed with error code: %d\n", rc);
		return rc;
	}
	return rc;
}

int pldm_send_msg_over_mctp(void *ctx, uint8_t tid, uint8_t *request, uint16_t req_len)
{
	int rc = mctp_tx(ctx, tid, request, req_len);
	if (rc != 0)
	{
		fprintf(stderr, "mctp_tx failed with error code: %d\n", rc);
		return rc;
	}
	return rc;
}

static int get_tid(void *ctx, uint8_t eid, uint8_t *tid)
{
	if (ctx == NULL || tid == NULL)
	{
		return -EINVAL;
	}

	uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
	memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

	uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
	memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

	uint8_t instance_id = get_instance_id();

	struct pldm_msg *request = (struct pldm_msg *)tbuf;
	uint16_t req_len = 3 + PLDM_GET_TID_REQ_BYTES;

	int rc = 0;
	rc = encode_get_tid_req(instance_id, request);

	struct pldm_msg *response = (struct pldm_msg *)rbuf;
	uint16_t response_len = 3 + PLDM_GET_TID_RESP_BYTES;

	rc = pldm_send_recv_msg_over_mctp(ctx, eid, (uint8_t *)request, req_len,
									  (uint8_t *)response, &response_len);
	if (!rc)
	{
		uint8_t cc;
		rc = decode_get_tid_resp(response, PLDM_GET_TID_RESP_BYTES, &cc, tid);
	}

	return rc;
}

static int set_tid(void *ctx, uint8_t eid, uint8_t tid)
{
	if (ctx == NULL || tid == 0 || tid == 0xff)
	{
		return -EINVAL;
	}

	uint8_t instance_id = get_instance_id();
	struct pldm_msg request = {0};

	int rc = 0;
	rc = encode_set_tid_req(instance_id, tid, &request);
	struct pldm_msg response = {0};

	uint16_t req_len = 3 + PLDM_SET_TID_REQ_BYTES;
	uint16_t response_len = 3 + PLDM_SET_TID_RESP_BYTES;

	rc = pldm_send_recv_msg_over_mctp(ctx, eid, (uint8_t *)&request, req_len,
									  (uint8_t *)&response, &response_len);
	if (rc)
	{
		fprintf(stderr, "decode_get_tid_resp failed with error code: %d\n", rc);
	}

	return rc;
}

static int get_pldm_types(void *ctx, uint8_t tid, bitfield8_t *supportedTypes)
{
	if (ctx == NULL || supportedTypes == NULL)
	{
		return -EINVAL;
	}

	uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
	memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

	struct pldm_msg request = {0};
	uint8_t instance_id = get_instance_id();
	uint16_t req_len = 3 + PLDM_GET_TYPES_REQ_BYTES;

	int rc = encode_get_types_req(instance_id, &request);
	struct pldm_msg *response = (struct pldm_msg *)rbuf;
	uint16_t response_len = 3 + PLDM_GET_TYPES_RESP_BYTES;

	rc = pldm_send_recv_msg_over_mctp(ctx, tid, (uint8_t *)&request, req_len,
									  (uint8_t *)response, &response_len);
	uint8_t cc;
	if (!rc)
	{
		rc = decode_get_types_resp(response, PLDM_GET_TYPES_RESP_BYTES, &cc, supportedTypes);
		// printPldmTypes((uint8_t *)response, response_len);
	}

	return rc;
}

static int get_pldm_version(void *ctx, uint8_t tid, uint8_t type, ver32_t *version)
{
	if (ctx == NULL || version == NULL)
	{
		return -EINVAL;
	}

	uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
	memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

	uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
	memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

	uint8_t instance_id = get_instance_id();

	struct pldm_msg *request = (struct pldm_msg *)tbuf;
	uint16_t req_len = 3 + PLDM_GET_VERSION_REQ_BYTES;

	int rc = encode_get_version_req(instance_id, 0, PLDM_GET_FIRSTPART, type, request);

	struct pldm_msg *response = (struct pldm_msg *)rbuf;
	uint16_t response_len = 3 + PLDM_GET_VERSION_RESP_BYTES;

	rc = pldm_send_recv_msg_over_mctp(ctx, tid, (uint8_t *)request, req_len,
									  (uint8_t *)response, &response_len);

	uint32_t transfer_handle;
	uint8_t transfer_flag;

	uint8_t cc;
	struct variable_field verp;
	rc = decode_get_version_resp(response, PLDM_GET_VERSION_RESP_BYTES, &cc, &transfer_handle, &transfer_flag,
								 &verp);

	memcpy(version, verp.ptr, verp.length);
	version->major = verp.ptr[3];
	version->minor = verp.ptr[2];
	version->update = verp.ptr[1];
	version->alpha = verp.ptr[0];

	return rc;
}

static int get_pldm_commands(void *ctx, uint8_t tid, uint8_t type, ver32_t version, bitfield8_t *supported_cmds)
{
	if (ctx == NULL || supported_cmds == NULL)
	{
		return -EINVAL;
	}

	uint8_t tbuf[PLDM_MAX_CMDS_PER_TYPE];
	memset(tbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

	uint8_t rbuf[PLDM_MAX_CMDS_PER_TYPE];
	memset(rbuf, 0, PLDM_MAX_CMDS_PER_TYPE);

	struct pldm_msg *request = (struct pldm_msg *)tbuf;
	uint16_t req_len = 3 + PLDM_GET_COMMANDS_REQ_BYTES;

	uint8_t instance_id = get_instance_id();

	int rc = encode_get_commands_req(instance_id, type, version, request);

	struct pldm_msg *response = (struct pldm_msg *)rbuf;
	uint16_t response_len = 3 + PLDM_GET_COMMANDS_RESP_BYTES;

	rc = pldm_send_recv_msg_over_mctp(ctx, tid, (uint8_t *)request, req_len,
									  (uint8_t *)response, &response_len);

	uint8_t cc;
	rc = decode_get_commands_resp(response, PLDM_GET_COMMANDS_RESP_BYTES, &cc, supported_cmds);

	return rc;
}

int pldm_init(void *mctp_ctx, pldm_ctx_t **ctx)
{
	uint8_t mctp_eid = 0;
	uint8_t tid = 0;
	pldm_ctx_t *pctx;

	if (mctp_ctx == NULL)
	{
		fprintf(stdout, "mctp_ctx is NULL\n");
		return -EINVAL;
	}

	pctx = malloc(sizeof(pldm_ctx_t));
	if (pctx == NULL)
	{
		fprintf(stdout, "malloc failed\n");
		return -ENOMEM;
	}

	pctx = malloc(sizeof(pldm_ctx_t));
	if (pctx == NULL)
	{
		fprintf(stdout, "malloc failed\n");
		return -ENOMEM;
	}

	*ctx = pctx;

	pctx->mctp_ctx = mctp_ctx;
	pctx->pldm_types_count = 0;
	pctx->tid = 0;
	memset(pctx->pldm_types, 0, sizeof(pldm_type_info_t) * PLDM_SUPPORTED_TYPES);

	char word[5];
	sprintf(word, "0x%x", tid);

	if (get_tid(mctp_ctx, mctp_eid, &tid) != 0)
	{
		return -EIO;
	}

	pctx->tid = tid;

	if (tid == 0)
	{
		if (set_tid(mctp_ctx, 0x10, 1) != 0)
		{
			return -EIO;
		}
	}

	bitfield8_t supportedTypes[8];

	if (get_pldm_types(mctp_ctx, tid, supportedTypes) != 0)
	{
		return -EIO;
	}

	ver32_t version;

	if (get_pldm_version(mctp_ctx, tid, PLDM_BASE, &version) == 0)
	{
		/* 		fprintf(stdout, "PLDM Base version: %d.%d.%d.%d\n", version.major & 0x0F,
						version.minor & 0x0F, version.update & 0x0F, version.alpha & 0x0F); */
		pctx->pldm_types[PLDM_BASE].version = version;
		pctx->pldm_types[PLDM_BASE].is_supported = true;
		pctx->pldm_types_count++;
	}

	if (get_pldm_version(mctp_ctx, tid, PLDM_FRU, &version) == 0)
	{
		/* 		fprintf(stdout, " PLDM FRU version: %d.%d.%d.%d\n", version.major & 0x0F,
						version.minor & 0x0F, version.update & 0x0F, version.alpha & 0x0F); */
		pctx->pldm_types[PLDM_FRU].version = version;
		pctx->pldm_types[PLDM_FRU].is_supported = true;
		pctx->pldm_types_count++;
	}

	if (get_pldm_version(mctp_ctx, tid, PLDM_FWUP, &version) == 0)
	{
		/* 		fprintf(stdout, "PLDM FWUP version: %d.%d.%d.%d\n", version.major & 0x0F,
						version.minor & 0x0F, version.update & 0x0F, version.alpha & 0x0F); */
		pctx->pldm_types[PLDM_FWUP].version = version;
		pctx->pldm_types[PLDM_FWUP].is_supported = true;
		pctx->pldm_types_count++;
	}

	if (get_pldm_version(mctp_ctx, tid, PLDM_OEM, &version) == 0)
	{
		/* fprintf(stdout, "PLDM OEM version: %d.%d.%d.%d\n", version.major & 0x0F,
				version.minor & 0x0F, version.update & 0x0F, version.alpha & 0x0F); */
		pctx->pldm_oem_type.version = version;
		pctx->pldm_oem_type.is_supported = true;
		pctx->pldm_types_count++;
	}

	bitfield8_t supported_cmds[32];

	// fprintf(stdout, " PLDM commands\n");

	for (int type = 0; type < PLDM_SUPPORTED_TYPES; type++)
	{
		if (pctx->pldm_types[type].is_supported == true)
		{
			// fprintf(stdout, " PLDM Type: %d\n", type);
			if (get_pldm_commands(mctp_ctx, tid, type, pctx->pldm_types[type].version, supported_cmds) == 0)
			{
				// buf_print("PLDM Commands", &(supported_cmds[0].byte), 32);
			}
		}
	}

	return 0;
}

int pldm_setup(const char *i2c_filename, uint8_t i2c_addr, uint8_t mctp_init_flag, uint8_t pldm_init_flag,
			   uint8_t pldm_oem_init_flag, uint8_t pldm_fru_init_flag, uint8_t pldm_mc_init_flag)
{
	int rc;

	if (i2c_filename == NULL || strlen(i2c_filename) == 0)
	{
		fprintf(stderr, "Invalid I2C filename\n");
		return -EINVAL;
	}
	if (i2c_addr == 0 || i2c_addr > 0x7F)
	{
		fprintf(stderr, "Invalid I2C address: %02x\n", i2c_addr);
		return -EINVAL;
	}
	if (mctp_init_flag == 0 && pldm_init_flag == 0 && pldm_oem_init_flag == 0 && pldm_fru_init_flag == 0 && 
		pldm_mc_init_flag == 0)
	{
		fprintf(stdout, "No initialization flags set, skipping PLDM setup\n");
		return -EINVAL;
	}

	if (mctp_init_flag)
	{
		rc = mctp_init(i2c_filename, i2c_addr, &mctp_ctx);
		if (rc != 0)
		{
			fprintf(stdout, "MCTP init failed\n");
			return rc;
		}

		if (mctp_ctx == NULL)
		{
			fprintf(stderr, "MCTP is not initialized\n");
			return -EINVAL;
		}
	}
	if (pldm_init_flag)
	{
		rc = pldm_init(mctp_ctx, &pldm_ctx);
		if (rc)
		{
			fprintf(stdout, "PLDM init failed\n");
			return rc;
		}

		if (pldm_ctx == NULL)
		{
			fprintf(stderr, "PLDM is failed\n");
			return -EINVAL;
		}
	}
	if (pldm_oem_init_flag)
	{
		rc = pldm_oem_init(pldm_ctx);
		if (rc != 0)
		{
			fprintf(stderr, "PLDM OEM init failed with error code: %d\n", rc);
			return rc;
		}
	}

	if (pldm_fru_init_flag)
	{
		rc = pldm_fru_init(pldm_ctx);
		if (rc != 0)
		{
			fprintf(stderr, "PLDM FRU init failed with error code: %d\n", rc);
			return rc;
		}
	}

	if (pldm_mc_init_flag)
	{
		rc = pldm_platform_init(pldm_ctx);
		if (rc != 0)
		{
			fprintf(stderr, "PLDM MC init failed with error code: %d\n", rc);
			return rc;
		}
	}

	return 0;
}

void pldm_info_print(pldm_ctx_t *ctx)
{
	if (ctx == NULL)
	{
		fprintf(stderr, "Invalid PLDM context\n");
		return;
	}

	fprintf(stdout, "PLDM Information:\n");
	fprintf(stdout, "TID: %d\n", ctx->tid);
	fprintf(stdout, "Supported PLDM Types Count: %d\n", ctx->pldm_types_count);

	int i;

	for (i = 0; i < PLDM_SUPPORTED_TYPES; i++)
	{
		if (ctx->pldm_types[i].is_supported)
		{
			fprintf(stdout, "PLDM Type %d: Version %d.%d.%d.%d\n", i,
					ctx->pldm_types[i].version.major & 0x0F,
					ctx->pldm_types[i].version.minor & 0x0F,
					ctx->pldm_types[i].version.update & 0x0F,
					ctx->pldm_types[i].version.alpha & 0x0F);
		}
	}
	fprintf(stdout, "PLDM Type 0x%x: Version %d.%d.%d.%d\n", 0x3F,
			ctx->pldm_oem_type.version.major & 0x0F,
			ctx->pldm_oem_type.version.minor & 0x0F,
			ctx->pldm_oem_type.version.update & 0x0F,
			ctx->pldm_oem_type.version.alpha & 0x0F);
}

void print_usage(char *program_name)
{
	if (program_name == NULL)
	{
		program_name = "amctool";
	}
	fprintf(stdout, "Usage: %s \n", program_name);
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "  -h, --help\t\tShow this help message\n");
	fprintf(stdout, "  -v, --verbose\t\tEnable verbose output\n");
	fprintf(stdout, "  -b, --brief\t\tEnable brief output\n");
	fprintf(stdout, "  -V, --version\t\tShow version information\n");
	fprintf(stdout, "  -i, --interactive\t\tEnter interactive mode\n");

	fprintf(stdout, "  -c, --config <filename>\tSpecify the config file\n");

	fprintf(stdout, "  -m, --mctp_init\t\tPerform MCTP init\n");
	fprintf(stdout, "  -p, --pldm_init\t\tPerform PLDM init\n");
	fprintf(stdout, "  -o, --pldm_oem_init\t\tPerform PLDM OEM init\n");
	fprintf(stdout, "  -f, --pldm_fru_init\t\tPerform PLDM FRU init\n");
	fprintf(stdout, "  -q, --pldm_mc_init\t\tPerform PLDM Platform init\n");
	fprintf(stdout, "  -g, --pkg_parse\t\tParse AMC FW Package\n");

	fprintf(stdout, "  -u, --fwu \t\t\tPerform firmware update\n");
	fprintf(stdout, "  -s, --vr_sync\t\t\tPerform PLDM VR Sync\n");
	fprintf(stdout, "  -n, --svn\t\t\tPerform PLDM SVN Query\n");
	fprintf(stdout, "  -e, --sensor_enable\t\t\tEnable Sensors\n");
	fprintf(stdout, "  -r, --reset_gpu\t\t\tReset GPU\n");

	fprintf(stdout, "  -M, --mctp_info\t\tDisplay MCTP Information\n");
	fprintf(stdout, "  -P, --pldm_info\t\tDisplay PLDM Information\n");
	fprintf(stdout, "  -G, --img_info\t\tDisplay FW Package Information\n");
	fprintf(stdout, "  -F, --fru_info\t\tDisplay FRU Information\n");
	fprintf(stdout, "  -Q, --pldm_mc_info\t\tDisplay MC Information\n");
}

static void print_version(void)
{
	fprintf(stdout, "AMC Tool version %d.%d.%d.%d\n", 
			AMCTOOL_MAJOR_VERSION, 
			AMCTOOL_MINOR_VERSION, 
			AMCTOOL_UPDATE_VERSION, 
			AMCTOOL_ALPHA_VERSION);
	fprintf(stdout, "Built on %s at %s\n", __DATE__, __TIME__);
}

int main(int argc, char **argv)
{
	cmd_options_t opts;
	int rc;

	// Initialize command line options
	init_cmd_options(&opts);

	// Parse command line arguments
	rc = parse_cmd_options(argc, argv, &opts);
	if (rc != 0) {
		return rc;
	}

	// Handle interactive mode
	if (opts.interactive_flag) {
		// Parse configuration file
		amc_config_t amc_config;
		parse_amc_config(opts.config_file, &amc_config);
		
		return interactive_mode(&amc_config);
	}

	// Validate options for non-interactive mode
	rc = validate_cmd_options(&opts);
	if (rc != 0) {
		return rc;
	}

	// Parse configuration file
	amc_config_t amc_config;
	parse_amc_config(opts.config_file, &amc_config);

	// Execute operations
	rc = execute_operations(&opts, &amc_config);
	if (rc != 0) {
		return rc;
	}

	// Execute info commands
	rc = execute_info_commands(&opts, &amc_config);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

static void init_cmd_options(cmd_options_t *opts)
{
	opts->verbose_flag = 0;
	opts->version_flag = 0;
	opts->interactive_flag = 0;
	opts->config_file[0] = '\0';

	// Initialization flags
	opts->mctp_init_flag = 0;
	opts->pldm_init_flag = 0;
	opts->pldm_oem_init_flag = 0;
	opts->pldm_fru_init_flag = 0;
	opts->pldm_mc_init_flag = 0;

	// Operation flags
	opts->parse_pkg_flag = 0;
	opts->fwu_flag = 0;
	opts->pldm_vr_sync_flag = 0;
	opts->pldm_svn_flag = 0;
	opts->pldm_sensor_enable_flag = 0;
	opts->pldm_aic_reset_flag = 0;

	// Info flags
	opts->mctp_info_flag = 0;
	opts->pldm_info_flag = 0;
	opts->pkg_info_flag = 0;
	opts->fru_info_flag = 0;
	opts->pldm_mc_info_flag = 0;
}

static int parse_cmd_options(int argc, char **argv, cmd_options_t *opts)
{
	int option_index = 0;
	int c;

	static struct option long_options[] = {
		{"verbose", no_argument, 0, 'v'},
		{"brief", no_argument, 0, 'b'},
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'V'},
		{"interactive", no_argument, 0, 'i'},
		{"config", required_argument, 0, 'c'},
		{"mctp_init", no_argument, 0, 'm'},
		{"pldm_init", no_argument, 0, 'p'},
		{"pldm_oem_init", no_argument, 0, 'o'},
		{"pldm_fru_init", no_argument, 0, 'f'},
		{"pldm_mc_init", no_argument, 0, 'q'},
		{"pkg_parse", no_argument, 0, 'g'},
		{"fwu", no_argument, 0, 'u'},
		{"vr_sync", no_argument, 0, 's'},
		{"svn", no_argument, 0, 'n'},
		{"sensor", no_argument, 0, 'e'},
		{"aic_reset", no_argument, 0, 'r'},
		{"mctp_info", no_argument, 0, 'M'},
		{"pldm_info", no_argument, 0, 'P'},
		{"pkg_info", no_argument, 0, 'G'},
		{"fru_info", no_argument, 0, 'F'},
		{"pldm_mc_info", no_argument, 0, 'Q'},
		{0, 0, 0, 0},
	};

	// Handle case with no arguments
	if (argc == 1) {
		print_usage(argv[0]);
		return -1;
	}

	while ((c = getopt_long(argc, argv, "vbhVic:mpofqgusnerMPGFQ", long_options, &option_index)) != -1)
	{
		if (c == -1) {
			break; // No more options
		}
		if (optarg && strlen(optarg) >= PATH_MAX) {
			fprintf(stderr, "Argument too long for option %c\n", c);
			return -1;
		}

		switch (c)
		{
		case 'h':
		case '?':
			print_usage(argv[0]);
			return 0;
		case 'v':
			opts->verbose_flag = 1;
			fprintf(stdout, "Verbose mode : %d\n", opts->verbose_flag);
			break;
		case 'V':
			print_version();
			return 0;
		case 'i':
			opts->interactive_flag = 1;
			break;
		case 'c':
			if (optarg) {
				strncpy(opts->config_file, optarg, sizeof(opts->config_file) - 1);
				opts->config_file[strlen(opts->config_file)] = '\0';
			} else {
				fprintf(stderr, "Config file not specified. Use --config <filename>\n");
				return -1;
			}
			break;
		case 'm':
			opts->mctp_init_flag = 1;
			break;
		case 'p':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			break;
		case 'o':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->pldm_oem_init_flag = 1;
			break;
		case 'f':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->pldm_fru_init_flag = 1;
			break;
		case 'q':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->pldm_mc_init_flag = 1;
			break;
		case 'g':
			opts->parse_pkg_flag = 1;
			break;
		case 'u':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->parse_pkg_flag = 1;
			opts->fwu_flag = 1;
			break;
		case 's':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->pldm_vr_sync_flag = 1;
			break;
		case 'n':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->pldm_svn_flag = 1;
			break;
		case 'e':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->pldm_mc_init_flag = 1;
			opts->pldm_sensor_enable_flag = 1;
			break;
		case 'r':
			opts->mctp_init_flag = 1;
			opts->pldm_init_flag = 1;
			opts->pldm_mc_init_flag = 1;
			opts->pldm_aic_reset_flag = 1;
			break;
		case 'M':
			opts->mctp_info_flag = 1;
			break;
		case 'P':
			opts->pldm_info_flag = 1;
			break;
		case 'G':
			opts->pkg_info_flag = 1;
			break;
		case 'F':
			opts->fru_info_flag = 1;
			break;
		case 'Q':
			opts->pldm_mc_info_flag = 1;
			break;
		default:
			fprintf(stderr, "Unknown option: %c\n", c);
			return -1;
		}
	}

	return 0;
}

static int validate_cmd_options(const cmd_options_t *opts)
{
	// Interactive mode doesn't need other operations
	if (opts->interactive_flag) {
		if (opts->config_file[0] == '\0') {
			fprintf(stderr, "Config file required for interactive mode. Use --config <filename>\n");
			return -1;
		}
		return 0;
	}

	// Check if any operation is specified
	if (!opts->mctp_init_flag && !opts->pldm_init_flag &&
		!opts->pldm_oem_init_flag && !opts->pldm_fru_init_flag && 
		!opts->parse_pkg_flag && !opts->fwu_flag && 
		!opts->pldm_vr_sync_flag && !opts->pldm_svn_flag && 
		!opts->pldm_mc_init_flag && !opts->pldm_sensor_enable_flag &&
		!opts->mctp_info_flag && !opts->pldm_info_flag &&
		!opts->pkg_info_flag && !opts->fru_info_flag && 
		!opts->pldm_mc_info_flag) {
		//fprintf(stderr, "No operation specified, exiting\n");
		return -EINVAL;
	}

	// Config file is required for most operations
	if (opts->config_file[0] == '\0') {
		fprintf(stderr, "Config file not specified. Use --config <filename>\n");
		return -1;
	}

	return 0;
}

static firmware_package_t firmware_package;

static int execute_operations(const cmd_options_t *opts, const amc_config_t *amc_config)
{
	int rc = 0;
	static int pldm_setup_done = 0;

	// Perform PLDM setup if needed
	if (opts->mctp_init_flag || opts->pldm_init_flag || 
		opts->pldm_oem_init_flag || opts->pldm_fru_init_flag || 
		opts->pldm_mc_init_flag) {
		
		rc = pldm_setup(amc_config->device, amc_config->slave_addr, 
						opts->mctp_init_flag, opts->pldm_init_flag,
						opts->pldm_oem_init_flag, opts->pldm_fru_init_flag, 
						opts->pldm_mc_init_flag);
		if (!rc) {
			pldm_setup_done = 1;
		}
	}

	// Handle package parsing
	if (opts->parse_pkg_flag) {
		if (amc_config->image_path[0] == '\0') {
			fprintf(stderr, "Firmware package path not specified in config\n");
			return -1;
		}

		if (pldm_setup_done == 0)
		{
			memset(&firmware_package, 0, sizeof(firmware_package_t));
			rc = fwpkg_parse_package_info(amc_config->image_path, &firmware_package);
			if (rc)
			{
				fprintf(stderr, "Package parse failed with error code: %d\n", rc);
				return rc;
			}
			return 0;
		}
		else
		{
			if (pldm_ctx == NULL)
			{
				fprintf(stderr, "PLDM is not initialized\n");
				return -EINVAL;
			}
			rc = fwpkg_parse_package_info(amc_config->image_path, &pldm_ctx->firmware_package);
			if (rc)
			{
				fprintf(stderr, "Package parse failed with error code: %d\n", rc);
				return rc;
			}
		}
	}

	// Handle firmware update
	if (opts->fwu_flag) {
		if (pldm_ctx == NULL) {
			fprintf(stderr, "PLDM is not initialized\n");
			return -EINVAL;
		}
		fprintf(stdout, "Firmware update file: %s\n", amc_config->image_path);
		fwupdate(pldm_ctx, amc_config->image_path);
	}

	// Handle VR sync
	if (opts->pldm_vr_sync_flag) {
		if (pldm_ctx == NULL) {
			fprintf(stderr, "PLDM is not initialized\n");
			return -EINVAL;
		}
		rc = pldm_oem_vr_sync(pldm_ctx);
		if (rc) {
			fprintf(stderr, "PLDM VR sync failed with error code: %d\n", rc);
			return rc;
		}
	}

	// Handle SVN query
	if (opts->pldm_svn_flag) {
		if (pldm_ctx == NULL) {
			fprintf(stderr, "PLDM is not initialized\n");
			return -EINVAL;
		}
		rc = pldm_oem_svn(pldm_ctx);
		if (rc) {
			fprintf(stderr, "PLDM SVN operation failed with error code: %d\n", rc);
			return rc;
		}
	}

	// Handle sensor enable
	if (opts->pldm_sensor_enable_flag) {
		if (pldm_ctx == NULL) {
			fprintf(stderr, "PLDM is not initialized\n");
			return -EINVAL;
		}
		get_all_state_sensors_reading(pldm_ctx);
		if (pldm_ctx->platform_ctx == NULL) {
			fprintf(stderr, "PLDM MC is not initialized\n");
			return -EINVAL;
		}
		rc = amc_sensors_enable_all(pldm_ctx);
		if (rc) {
			fprintf(stderr, "Failed to enable all sensors with error code: %d\n", rc);
			return rc;
		}
		rc = get_all_numeric_sensor_readings(pldm_ctx);
		if (rc) {
			fprintf(stderr, "Failed to get all numeric sensor readings with error code: %d\n", rc);
			return rc;
		}
	}

	return 0;
}

static int execute_info_commands(const cmd_options_t *opts, const amc_config_t *amc_config)
{
	// Handle MCTP info
	if (opts->mctp_info_flag) {
		if (mctp_ctx == NULL) {
			fprintf(stderr, "MCTP context is not initialized\n");
			return -EINVAL;
		}
		mctp_info_print(mctp_ctx);
	}

	// Handle PLDM info
	if (opts->pldm_info_flag) {
		if (pldm_ctx == NULL) {
			fprintf(stderr, "PLDM context is not initialized\n");
			return -EINVAL;
		}
		pldm_info_print(pldm_ctx);
	}

	// Handle package info
	if (opts->pkg_info_flag) {
		if (pldm_ctx != NULL) {
			pkg_info_print(&pldm_ctx->firmware_package);
		} else {
			pkg_info_print(&firmware_package);
		}
	}

	// Handle FRU info
	if (opts->fru_info_flag) {
		if (pldm_ctx == NULL || pldm_ctx->fru_ctx == NULL) {
			fprintf(stderr, "FRU context is not initialized\n");
			return -EINVAL;
		}
		fru_info_print(pldm_ctx);
	}

	// Handle MC info
	if (opts->pldm_mc_info_flag) {
		if (pldm_ctx == NULL || pldm_ctx->platform_ctx == NULL) {
			fprintf(stderr, "PLDM Platform context is not initialized\n");
			return -EINVAL;
		}
		pldm_platform_info_print(pldm_ctx);
	}

	return 0;
}

static void print_interactive_help(void)
{
	fprintf(stdout, "\nInteractive Mode Commands:\n");
	fprintf(stdout, "  help, h               - Show this help message\n");
	fprintf(stdout, "  quit, exit, q         - Exit interactive mode\n");
	fprintf(stdout, "  version, ver          - Show version information\n");
	fprintf(stdout, "\nInitialization Commands:\n");
	fprintf(stdout, "  mctp_init             - Perform MCTP initialization\n");
	fprintf(stdout, "  pldm_init             - Perform PLDM initialization\n");
	fprintf(stdout, "  pldm_oem_init         - Perform PLDM OEM initialization\n");
	fprintf(stdout, "  pldm_fru_init         - Perform PLDM FRU initialization\n");
	fprintf(stdout, "  pldm_mc_init          - Perform PLDM Platform initialization\n");
	fprintf(stdout, "\nOperation Commands:\n");
	fprintf(stdout, "  pkg_parse             - Parse AMC FW Package\n");
	fprintf(stdout, "  fwu                   - Perform firmware update\n");
	fprintf(stdout, "  vr_sync               - Perform PLDM VR Sync\n");
	fprintf(stdout, "  svn                   - Perform PLDM SVN Query\n");
	fprintf(stdout, "  sensor_enable         - Enable Sensors\n");
	fprintf(stdout, "  reset_gpu             - Reset GPU\n");
	fprintf(stdout, "\nInformation Commands:\n");
	fprintf(stdout, "  mctp_info             - Display MCTP Information\n");
	fprintf(stdout, "  pldm_info             - Display PLDM Information\n");
	fprintf(stdout, "  pkg_info              - Display FW Package Information\n");
	fprintf(stdout, "  fru_info              - Display FRU Information\n");
	fprintf(stdout, "  pldm_mc_info          - Display MC Information\n");
	fprintf(stdout, "\n");
}

static int parse_interactive_command(const char *input, cmd_options_t *opts)
{
	// Reset all flags except interactive and config
	int interactive_flag = opts->interactive_flag;
	char config_file[PATH_MAX];
	strncpy(config_file, opts->config_file, sizeof(config_file) - 1);
	config_file[sizeof(config_file) - 1] = '\0';
	
	init_cmd_options(opts);
	opts->interactive_flag = interactive_flag;
	strncpy(opts->config_file, config_file, sizeof(opts->config_file) - 1);
	opts->config_file[sizeof(opts->config_file) - 1] = '\0';
	
	// Remove newline if present
	char *newline = strchr(input, '\n');
	if (newline) *newline = '\0';
	
	// Trim whitespace
	while (*input == ' ' || *input == '\t') input++;
	
	if (strlen(input) == 0) {
		return 0; // Empty command
	}
	
	// Parse commands
	if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
		print_interactive_help();
		return 0;
	}
	else if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0 || strcmp(input, "q") == 0) {
		return -1; // Signal to exit
	}
	else if (strcmp(input, "version") == 0 || strcmp(input, "ver") == 0) {
		print_version();
		return 0;
	}
	else if (strcmp(input, "mctp_init") == 0) {
		opts->mctp_init_flag = 1;
		return 1; // Signal to execute
	}
	else if (strcmp(input, "pldm_init") == 0) {
		opts->pldm_init_flag = 1;
		return 1;
	}
	else if (strcmp(input, "pldm_oem_init") == 0) {
		opts->pldm_oem_init_flag = 1;
		return 1;
	}
	else if (strcmp(input, "pldm_fru_init") == 0) {
		opts->pldm_fru_init_flag = 1;
		return 1;
	}
	else if (strcmp(input, "pldm_mc_init") == 0) {
		opts->pldm_mc_init_flag = 1;
		return 1;
	}
	else if (strcmp(input, "pkg_parse") == 0) {
		opts->parse_pkg_flag = 1;
		return 1;
	}
	else if (strcmp(input, "fwu") == 0) {
		opts->fwu_flag = 1;
		return 1;
	}
	else if (strcmp(input, "vr_sync") == 0) {
		opts->pldm_vr_sync_flag = 1;
		return 1;
	}
	else if (strcmp(input, "svn") == 0) {
		opts->pldm_svn_flag = 1;
		return 1;
	}
	else if (strcmp(input, "sensor_enable") == 0) {
		opts->pldm_sensor_enable_flag = 1;
		return 1;
	}
	else if (strcmp(input, "reset_gpu") == 0) {
		opts->pldm_aic_reset_flag = 1;
		return 1;
	}
	else if (strcmp(input, "mctp_info") == 0) {
		opts->mctp_info_flag = 1;
		return 1;
	}
	else if (strcmp(input, "pldm_info") == 0) {
		opts->pldm_info_flag = 1;
		return 1;
	}
	else if (strcmp(input, "pkg_info") == 0) {
		opts->pkg_info_flag = 1;
		return 1;
	}
	else if (strcmp(input, "fru_info") == 0) {
		opts->fru_info_flag = 1;
		return 1;
	}
	else if (strcmp(input, "pldm_mc_info") == 0) {
		opts->pldm_mc_info_flag = 1;
		return 1;
	}
	else {
		fprintf(stderr, "Unknown command: %s\n", input);
		fprintf(stderr, "Type 'help' for available commands.\n");
		return 0;
	}
}

static int interactive_mode(const amc_config_t *amc_config)
{
	char input[256];
	cmd_options_t opts;
	int rc;
	
	fprintf(stdout, "\nAMC Tool Interactive Mode\n");
	fprintf(stdout, "Type 'help' for available commands, 'quit' to exit.\n\n");
	
	while (1) {
		fprintf(stdout, "amctool> ");
		fflush(stdout);
		
		if (fgets(input, sizeof(input), stdin) == NULL) {
			// EOF or error
			fprintf(stdout, "\n");
			break;
		}
		
		rc = parse_interactive_command(input, &opts);
		if (rc == -1) {
			// User wants to quit
			fprintf(stdout, "Exiting interactive mode.\n");
			break;
		}
		else if (rc == 1) {
			// Execute the command
			rc = execute_operations(&opts, amc_config);
			if (rc != 0) {
				fprintf(stderr, "Command execution failed with error code: %d\n", rc);
			}
			
			rc = execute_info_commands(&opts, amc_config);
			if (rc != 0) {
				fprintf(stderr, "Info command execution failed with error code: %d\n", rc);
			}
		}
		// rc == 0 means command was handled (like help) but no execution needed
	}
	
	return 0;
}
