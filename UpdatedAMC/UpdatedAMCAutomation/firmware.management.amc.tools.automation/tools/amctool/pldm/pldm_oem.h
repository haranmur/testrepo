#ifndef PLDM_OEM_H
#define PLDM_OEM_H
#include "../libpldm/base.h"

#define PLDM_OEM_TYPE                           0x3F

#define PLDM_OEM_VR_SYNC_PAUSE          0x01
#define PLDM_OEM_VR_SYNC_RESUME       0x02

#define PLDM_OEM_PLOD_IS_SUPPORTED   0x03
#define PLDM_OEM_PLOG_STATUS              0x04
#define PLDM_OEM_PLOG_SIZE                  0x05
#define PLDM_OEM_PLOG_SESSION_ID       0x06
#define PLDM_OEM_PLOG_READ_DATA        0x07               

#define PLDM_OEM_GET_SVN                     0x08
#define PLDM_OEM_COMMIT_SVN               0x09

#define PLDM_OEM_GET_VERSION_REQ_BYTES 0x00
#define PLDM_OEM_VR_SYNC_REQ_BYTES 0x00
#define PLDM_OEM_VR_RESUME_REQ_BYTES 0x00

#define PLDM_OEM_GET_VERSION_RESP_BYTES sizeof(struct pldm_oem_get_version_resp)
#define PLDM_OEM_VR_SYNC_RESP_BYTES sizeof(struct pldm_oem_get_version_resp)
#define PLDM_OEM_VR_RESUME_RESP_BYTES sizeof(struct pldm_oem_get_version_resp)

struct pldm_oem_svn
{
    union { 
        uint8_t hw_svn:8;
        uint8_t sw_svn:8;
        uint16_t reserved;
        uint32_t value;
    };
} __attribute__((packed));

struct pldm_commit_svn_req {
 struct pldm_oem_svn svn;
} __attribute__((packed));

struct pldm_commit_svn_resp {
    uint8_t completion_code;
   struct pldm_oem_svn svn;
} __attribute__((packed));

struct pldm_get_svn_resp {
    uint8_t completion_code;
    struct pldm_oem_svn svn;
} __attribute__((packed));

int encode_pldm_oem_vr_sync_pause_req(uint8_t instance_id,
                                    struct pldm_msg *msg);
int decode_pldm_oem_vr_sync_pause_resp(const struct pldm_msg *msg,
                                    size_t payload_length,
                                    uint8_t *cc);

int encode_pldm_oem_vr_sync_resume_req(uint8_t instance_id, struct pldm_msg *msg);
int decode_pldm_oem_vr_sync_resume_resp(const struct pldm_msg *msg, 
                                    size_t payload_length,
                                    uint8_t *cc);

int encode_pldm_oem_get_svn_req(uint8_t instance_id, struct pldm_msg *msg);
int decode_pldm_oem_get_svn_resp(const struct pldm_msg *msg,
                                    size_t payload_length,
                                    uint8_t *cc,   struct pldm_oem_svn *svn);

int encode_pldm_oem_commit_svn_req(uint8_t instance_id, 
                                    struct pldm_msg *msg);
int decode_pldm_oem_commit_svn_resp(const struct pldm_msg *msg,
                                    size_t payload_length,
                                    uint8_t *cc);

int pldm_oem_init(pldm_ctx_t *ctx);
int pldm_oem_vr_sync(pldm_ctx_t *ctx) ;
int pldm_oem_svn(pldm_ctx_t *ctx) ;

#endif // PLDM_OEM_H
