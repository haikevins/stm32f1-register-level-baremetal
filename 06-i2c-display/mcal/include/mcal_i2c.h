#ifndef MCAL_I2C_H
#define MCAL_I2C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    MCAL_I2C_INSTANCE_1 = 0,
    MCAL_I2C_INSTANCE_COUNT
} mcal_i2c_instance_t;

bool mcal_i2c_init(mcal_i2c_instance_t instance,
                   uint32_t peripheral_clock_hz,
                   uint32_t bus_clock_hz);

bool mcal_i2c_master_write_prefixed(mcal_i2c_instance_t instance,
                                    uint8_t address_7bit,
                                    uint8_t prefix,
                                    const uint8_t *data,
                                    size_t length);

#endif
