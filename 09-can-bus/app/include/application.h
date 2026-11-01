#ifndef APPLICATION_H
#define APPLICATION_H
#include <stdint.h>
extern volatile uint32_t application_can_tx_ok;
extern volatile uint32_t application_can_tx_failed;
extern volatile uint32_t application_can_tx_busy;
extern volatile uint32_t application_can_rx_ok;
extern volatile uint32_t application_can_rx_bad;
extern volatile uint32_t application_can_rx_overruns;
extern volatile uint32_t application_can_esr;
extern volatile uint8_t application_can_last_counter;
void application_init(void);
void application_process(void);
#endif
