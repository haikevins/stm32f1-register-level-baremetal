#include "application.h"
#include "application_config.h"
#include "can_service.h"
#include "time_service.h"
#include "indication_service.h"

volatile uint32_t application_can_tx_ok;
volatile uint32_t application_can_tx_failed;
volatile uint32_t application_can_tx_busy;
volatile uint32_t application_can_rx_ok;
volatile uint32_t application_can_rx_bad;
volatile uint32_t application_can_rx_overruns;
volatile uint32_t application_can_esr;
volatile uint8_t application_can_last_counter;

static uint8_t g_tx_counter;
static uint32_t g_last_send_ms;

void application_init(void)
{
    g_tx_counter = 0U;
    g_last_send_ms = time_service_now_ms();
    indication_service_set(INDICATION_STATUS, false);
    application_can_tx_ok = 0U;
    application_can_tx_failed = 0U;
    application_can_tx_busy = 0U;
    application_can_rx_ok = 0U;
    application_can_rx_bad = 0U;
    application_can_rx_overruns = 0U;
    application_can_esr = 0U;
    application_can_last_counter = 0U;
}

void application_process(void)
{
    can_service_stats_t s;
    can_service_process();
#if APPLICATION_CAN_ROLE == APPLICATION_CAN_ROLE_TX
    if (time_service_periodic_due(&g_last_send_ms,
                                   APPLICATION_CAN_PERIOD_MS)) {
        if (can_service_send(g_tx_counter)) { ++g_tx_counter; }
    }
#endif
    s = can_service_stats();
    application_can_tx_ok = s.tx_ok;
    application_can_tx_failed = s.tx_failed;
    application_can_tx_busy = s.tx_busy;
    application_can_rx_ok = s.rx_ok;
    application_can_rx_bad = s.rx_bad;
    application_can_rx_overruns = s.rx_overruns;
    application_can_esr = s.can_esr;
    application_can_last_counter = s.last_rx_counter;
}
