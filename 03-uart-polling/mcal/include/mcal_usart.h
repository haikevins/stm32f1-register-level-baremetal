#ifndef MCAL_USART_H
#define MCAL_USART_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MCAL_USART_INSTANCE_1 = 0,
    MCAL_USART_INSTANCE_COUNT
} mcal_usart_instance_t;

#define MCAL_USART_ERROR_PARITY  (UINT32_C(1) << 0U)
#define MCAL_USART_ERROR_FRAMING (UINT32_C(1) << 1U)
#define MCAL_USART_ERROR_NOISE   (UINT32_C(1) << 2U)
#define MCAL_USART_ERROR_OVERRUN (UINT32_C(1) << 3U)

bool mcal_usart_init(mcal_usart_instance_t instance,
                     uint32_t peripheral_clock_hz,
                     uint32_t baud_rate);
bool mcal_usart_try_read_byte(mcal_usart_instance_t instance, uint8_t *byte);
bool mcal_usart_try_write_byte(mcal_usart_instance_t instance, uint8_t byte);
uint32_t mcal_usart_take_error_flags(mcal_usart_instance_t instance);

#endif
