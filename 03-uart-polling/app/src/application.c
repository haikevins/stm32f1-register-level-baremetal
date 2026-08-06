#include "application.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application_config.h"
#include "serial_service.h"

static const uint8_t g_greeting[] = APPLICATION_UART_GREETING;

static size_t g_greeting_index;
static bool g_echo_pending;
static uint8_t g_echo_byte;

volatile uint8_t application_uart_last_rx_byte;
volatile uint32_t application_uart_rx_count;
volatile uint32_t application_uart_tx_count;
volatile uint32_t application_uart_error_events;
volatile uint32_t application_uart_error_flags;

void application_init(void)
{
    g_greeting_index = 0U;
    g_echo_pending = false;
    g_echo_byte = 0U;

    application_uart_last_rx_byte = 0U;
    application_uart_rx_count = 0U;
    application_uart_tx_count = 0U;
    application_uart_error_events = 0U;
    application_uart_error_flags = 0U;
}

void application_process(void)
{
    const uint32_t error_flags = serial_service_take_error_flags();

    if (error_flags != 0U)
    {
        application_uart_error_flags |= error_flags;
        application_uart_error_events++;
    }

    if (g_greeting_index < (sizeof(g_greeting) - 1U))
    {
        if (serial_service_try_write_byte(g_greeting[g_greeting_index]))
        {
            g_greeting_index++;
            application_uart_tx_count++;
        }

        return;
    }

    if (g_echo_pending)
    {
        if (serial_service_try_write_byte(g_echo_byte))
        {
            g_echo_pending = false;
            application_uart_tx_count++;
        }

        return;
    }

    uint8_t received_byte;

    if (serial_service_try_read_byte(&received_byte))
    {
        application_uart_last_rx_byte = received_byte;
        application_uart_rx_count++;

        g_echo_byte = received_byte;
        g_echo_pending = true;
    }
}
