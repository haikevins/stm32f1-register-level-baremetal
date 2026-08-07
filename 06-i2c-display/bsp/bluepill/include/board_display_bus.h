#ifndef BOARD_DISPLAY_BUS_H
#define BOARD_DISPLAY_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool board_display_bus_init(uint32_t peripheral_clock_hz);

bool board_display_bus_write(bool data_mode,
                             const uint8_t *data,
                             size_t length);

void board_display_bus_delay_ms(uint32_t delay_ms);

#endif
