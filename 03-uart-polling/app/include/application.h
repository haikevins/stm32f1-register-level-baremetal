#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>

extern volatile uint8_t application_uart_last_rx_byte;
extern volatile uint32_t application_uart_rx_count;
extern volatile uint32_t application_uart_tx_count;
extern volatile uint32_t application_uart_error_events;
extern volatile uint32_t application_uart_error_flags;

void application_init(void);
void application_process(void);

#endif
