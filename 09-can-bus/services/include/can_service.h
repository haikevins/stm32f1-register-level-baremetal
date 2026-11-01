#ifndef CAN_SERVICE_H
#define CAN_SERVICE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t tx_ok;
    uint32_t tx_failed;
    uint32_t tx_busy;
    uint32_t rx_ok;
    uint32_t rx_bad;
    uint32_t rx_overruns;
    uint32_t can_esr;
    uint8_t last_rx_counter;
    uint8_t last_rx_value;
} can_service_stats_t;
void can_service_init(void);
void can_service_process(void);
bool can_service_send(uint8_t counter);
can_service_stats_t can_service_stats(void);
#endif
