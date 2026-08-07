#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ   (72000000UL)
#define BOARD_UART_BAUD_RATE     (115200UL)

#if BOARD_UART_BAUD_RATE == 0UL
#error "BOARD_UART_BAUD_RATE must be greater than zero."
#endif

#endif
