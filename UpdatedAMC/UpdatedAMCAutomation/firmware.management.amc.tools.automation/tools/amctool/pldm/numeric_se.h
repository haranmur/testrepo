#ifndef NUMERIC_SE_H
#define NUMERIC_SE_H

#include <stdint.h>
#include "../libpldm/base.h"
#include "../libpldm/platform.h"
#include "pldm.h"

int get_all_numeric_sensor_readings(void *ctx);
int amc_sensors_enable_all(void *pctx);

#endif // NUMERIC_SE_H