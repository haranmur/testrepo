#ifndef STATE_SE_H
#define STATE_SE_H
#include <stdint.h>

#define PLDM_STATE_EFFECTER_OEM_FRU_LOCK 401
#define PLDM_STATE_EFFECTER_AIC_RESET       402

#define PLDM_FRU_LOCK (0)
#define PLDM_FRU_UNLOCK (1)

int get_all_state_sensors_reading(void *ctx);
int get_fru_lock_state(void *ctx);
int set_fru_lock_state(void *ctx, bool lock);
int aic_reset(void *pctx);

#endif // STATE_SE_H