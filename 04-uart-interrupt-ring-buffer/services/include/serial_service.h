#ifndef SERIAL_SERVICE_H
#define SERIAL_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

void serial_service_init(void);
bool serial_service_try_read_byte(uint8_t *byte);
bool serial_service_try_write_byte(uint8_t byte);
uint32_t serial_service_take_error_flags(void);
uint32_t serial_service_take_rx_overflow_count(void);

#endif
