#ifndef PLDM_MC_H
#define PLDM_MC_H

#include <stdint.h>
#include "../libpldm/platform.h"
#include "numeric_se.h"
#include "state_se.h"

typedef struct pldm_ctx pldm_ctx_t;

struct id2name_map {
    uint16_t id;
    uint8_t tag_length;
    uint8_t tag[32];
    uint8_t auxiliary_name_length;
    uint16_t auxiliary_name[256];
};

typedef struct {
    uint8_t terminus_uid[UUID_SIZE];
    
	struct pldm_pdr_repository_info pdr_repository_info;

    int total_pdrs;
	struct pldm_pdr *pdrs;
    
    int terminus_locator_record_count;
    struct pldm_terminus_locator_pdr **terminus_locator_pdr;

    int numeric_sensor_record_count;
    struct pldm_numeric_sensor_value_pdr **numeric_sensor_pdr;
    struct id2name_map *numeric_sensor_id2name_map;
    int numeric_sensor_id2name_map_count;
    
    int state_sensor_record_count;
    struct pldm_state_sensor_pdr **state_sensor_pdr;
    struct id2name_map *state_sensor_id2name_map;
    int state_sensor_id2name_map_count;

    int sensor_auxiliary_names_record_count;
    struct pldm_sensor_auxiliary_names_pdr **sensor_auxiliary_names_pdr;

    int numeric_effecter_record_count;
    struct pldm_numeric_effecter_value_pdr **numeric_effecter_pdr;
    struct id2name_map *numeric_effecter_id2name_map;
    int numeric_effecter_id2name_map_count;

    int state_effecter_record_count;
    struct pldm_state_effecter_pdr **state_effecter_pdr;
    struct id2name_map *state_effecter_id2name_map;
    int state_effecter_id2name_map_count;

    int effecter_auxiliary_names_record_count;
    struct pldm_effecter_auxiliary_names_pdr **effecter_auxiliary_names_pdr;

    int entity_association_record_count;
    struct pldm_pdr_entity_association **entity_association_pdr;

    int entity_auxiliary_names_record_count;
    struct pldm_pdr_entity_auxiliary_names **entity_auxiliary_names;

    int fru_record_set_record_count;
    struct pldm_fru_record_set_pdr **fru_record_set_pdr;
}pldm_platform_ctx_t;

void pldm_platform_info_print(pldm_ctx_t *ctx);
int pldm_platform_init(pldm_ctx_t *ctx);

#endif // PLDM_MC_H
