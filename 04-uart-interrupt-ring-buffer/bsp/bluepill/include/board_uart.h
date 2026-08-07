#ifndef BOARD_UART_H
#define BOARD_UART_H

#include <stdbool.h>
#include <stdint.h>

bool board_uart_init(uint32_t peripheral_clock_hz);
bool board_uart_try_read_byte(uint8_t *byte);
bool board_uart_try_write_byte(uint8_t byte);
uint32_t board_uart_take_error_flags(void);
uint32_t board_uart_take_rx_overflow_count(void);

#endif
