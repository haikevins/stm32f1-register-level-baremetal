#ifndef MCAL_SPI_H
#define MCAL_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    MCAL_SPI_INSTANCE_1 = 0
} mcal_spi_instance_t;

typedef enum
{
    MCAL_SPI_MODE_0 = 0,
    MCAL_SPI_MODE_3
} mcal_spi_mode_t;

bool mcal_spi_init(mcal_spi_instance_t instance,
                   uint32_t peripheral_clock_hz,
                   uint32_t maximum_spi_clock_hz,
                   mcal_spi_mode_t mode);

bool mcal_spi_transfer(mcal_spi_instance_t instance,
                       const uint8_t *transmit_data,
                       uint8_t *receive_data,
                       size_t length);

#endif
