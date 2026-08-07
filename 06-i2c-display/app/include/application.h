#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>
#include <stdint.h>

extern volatile uint8_t application_display_progress_percent;
extern volatile uint32_t application_display_update_count;
extern volatile uint32_t application_display_error_count;

bool application_init(void);
void application_process(void);

#endif
