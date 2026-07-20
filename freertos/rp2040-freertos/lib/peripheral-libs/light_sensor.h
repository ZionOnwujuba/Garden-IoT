#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "hardware/i2c.h"

void bh1750_init(i2c_inst_t *i2c);

uint16_t bh1750_read_light(i2c_inst_t *i2c);

#endif