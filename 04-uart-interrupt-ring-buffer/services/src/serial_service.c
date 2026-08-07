#include "serial_service.h"

#include "board_uart.h"

void serial_service_init(void)
{
    (void)board_uart_take_error_flags();
    (void)board_uart_take_rx_overflow_count();
}

bool serial_service_try_read_byte(uint8_t *byte)
{
    return board_uart_try_read_byte(byte);
}

bool serial_service_try_write_byte(uint8_t byte)
{
    return board_uart_try_write_byte(byte);
}

uint32_t serial_service_take_error_flags(void)
{
    return board_uart_take_error_flags();
}

uint32_t serial_service_take_rx_overflow_count(void)
{
    return board_uart_take_rx_overflow_count();
}
