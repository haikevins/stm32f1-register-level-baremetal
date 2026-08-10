#ifndef BOARD_MEMORY_BUS_H
#define BOARD_MEMORY_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool board_memory_bus_init(uint32_t peripheral_clock_hz);

bool board_memory_bus_transfer(const uint8_t *transmit_data,
                               uint8_t *receive_data,
                               size_t length);

void board_memory_bus_select(void);
void board_memory_bus_deselect(void);
void board_memory_bus_delay_ms(uint32_t delay_ms);

#endif
