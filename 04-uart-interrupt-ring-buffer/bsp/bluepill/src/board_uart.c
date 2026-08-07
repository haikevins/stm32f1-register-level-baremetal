#include "board_uart.h"

#include "board_config.h"
#include "board_pins.h"
#include "mcal_gpio.h"
#include "mcal_usart.h"

bool board_uart_init(uint32_t peripheral_clock_hz)
{
    if (!mcal_gpio_configure(BOARD_UART_TX_PORT,
                             BOARD_UART_TX_PIN,
                             MCAL_GPIO_MODE_AF_PP_50MHZ,
                             MCAL_GPIO_LEVEL_HIGH))
    {
        return false;
    }

    if (!mcal_gpio_configure(BOARD_UART_RX_PORT,
                             BOARD_UART_RX_PIN,
                             MCAL_GPIO_MODE_INPUT_FLOATING,
                             MCAL_GPIO_LEVEL_LOW))
    {
        return false;
    }

    return mcal_usart_init(MCAL_USART_INSTANCE_1,
                           peripheral_clock_hz,
                           BOARD_UART_BAUD_RATE);
}

bool board_uart_try_read_byte(uint8_t *byte)
{
    return mcal_usart_try_read_byte(MCAL_USART_INSTANCE_1, byte);
}

bool board_uart_try_write_byte(uint8_t byte)
{
    return mcal_usart_try_write_byte(MCAL_USART_INSTANCE_1, byte);
}

uint32_t board_uart_take_error_flags(void)
{
    return mcal_usart_take_error_flags(MCAL_USART_INSTANCE_1);
}

uint32_t board_uart_take_rx_overflow_count(void)
{
    return mcal_usart_take_rx_overflow_count(MCAL_USART_INSTANCE_1);
}
