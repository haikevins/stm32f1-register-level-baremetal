#include "board_can.h"
#include <stddef.h>
#include "board_config.h"
#include "board_pins.h"
#include "mcal_gpio.h"
#include "mcal_irq.h"
#include "mcal_nvic.h"
#include "mcal_can.h"

static can_frame_t g_received;
static volatile bool g_received_ready;
static volatile uint32_t g_rx_overruns;

bool board_can_init(uint32_t apb1_clock_hz)
{
    g_received_ready = false;
    g_rx_overruns = 0U;
    if (!mcal_gpio_configure(BOARD_CAN_RX_PORT, BOARD_CAN_RX_PIN,
                             MCAL_GPIO_MODE_INPUT_FLOATING, MCAL_GPIO_LEVEL_LOW)) {
        return false;
    }
    if (!mcal_gpio_configure(BOARD_CAN_TX_PORT, BOARD_CAN_TX_PIN,
                             MCAL_GPIO_MODE_AF_PP_50MHZ, MCAL_GPIO_LEVEL_HIGH)) {
        return false;
    }
    if (!mcal_can_init(apb1_clock_hz, BOARD_CAN_BITRATE_HZ, 0x123U,
                       BOARD_CAN_INIT_TIMEOUT)) {
        return false;
    }
    return mcal_nvic_enable_irq(MCAL_NVIC_IRQ_CAN1_RX0,
                                 BOARD_CAN_RX_IRQ_PRIORITY);
}

bool board_can_transmit(const can_frame_t *frame)
{
    return mcal_can_transmit(frame);
}

board_can_tx_state_t board_can_poll_tx(void)
{
    return (board_can_tx_state_t)mcal_can_poll_tx();
}

bool board_can_take_received(can_frame_t *frame)
{
    uint32_t primask;
    if (frame == NULL) { return false; }
    primask = mcal_irq_save_and_disable();
    if (!g_received_ready) {
        mcal_irq_restore(primask);
        return false;
    }
    *frame = g_received;
    g_received_ready = false;
    mcal_irq_restore(primask);
    return true;
}

uint32_t board_can_get_overruns(void) { return g_rx_overruns; }
uint32_t board_can_get_error_status(void) { return mcal_can_get_esr(); }

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    can_frame_t frame;
    /* FIFO0 has at most 3 frames, so ISR work is bounded. */
    uint32_t count;
    for (count = 0U; count < 3U; ++count) {
        if (!mcal_can_pop_fifo0(&frame)) { break; }
        if (g_received_ready) { ++g_rx_overruns; }
        g_received = frame;
        g_received_ready = true;
    }
}
