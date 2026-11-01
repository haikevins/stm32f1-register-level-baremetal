#include "can_service.h"
#include "board_can.h"
#include "indication_service.h"

static can_service_stats_t g_stats;

void can_service_init(void)
{
    /* Explicit scalar stores: no libc memset dependency in freestanding build. */
    g_stats.tx_ok = 0U;
    g_stats.tx_failed = 0U;
    g_stats.tx_busy = 0U;
    g_stats.rx_ok = 0U;
    g_stats.rx_bad = 0U;
    g_stats.rx_overruns = 0U;
    g_stats.can_esr = 0U;
    g_stats.last_rx_counter = 0U;
    g_stats.last_rx_value = 0U;
}

bool can_service_send(uint8_t counter)
{
    const can_frame_t frame = {
        .id = 0x123U,
        .dlc = 2U,
        .data = {counter, 0x11U, 0U, 0U, 0U, 0U, 0U, 0U}
    };
    if (!board_can_transmit(&frame)) {
        ++g_stats.tx_busy;
        return false;
    }
    return true;
}

void can_service_process(void)
{
    can_frame_t frame;
    const board_can_tx_state_t state = board_can_poll_tx();
    if (state == BOARD_CAN_TX_OK) { ++g_stats.tx_ok; }
    else if (state == BOARD_CAN_TX_ERROR) { ++g_stats.tx_failed; }

    if (board_can_take_received(&frame)) {
        if ((frame.id == 0x123U) && (frame.dlc == 2U) &&
            (frame.data[1] == 0x11U)) {
            ++g_stats.rx_ok;
            g_stats.last_rx_counter = frame.data[0];
            g_stats.last_rx_value = frame.data[1];
            indication_service_toggle(INDICATION_STATUS);
        } else {
            ++g_stats.rx_bad;
        }
    }
    g_stats.rx_overruns = board_can_get_overruns();
    g_stats.can_esr = board_can_get_error_status();
}

can_service_stats_t can_service_stats(void)
{
    return g_stats;
}
