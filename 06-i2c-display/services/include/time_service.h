#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

void time_service_init(void);
uint32_t time_service_now_ms(void);
uint32_t time_service_elapsed_ms(uint32_t start_time_ms);
bool time_service_periodic_due(uint32_t *last_run_ms, uint32_t period_ms);

#endif
