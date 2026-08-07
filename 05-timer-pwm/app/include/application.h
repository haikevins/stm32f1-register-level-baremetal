#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>
#include <stdint.h>

extern volatile uint16_t application_pwm_duty_permille;
extern volatile uint32_t application_pwm_update_count;
extern volatile bool application_pwm_ramping_up;

void application_init(void);
void application_process(void);

#endif
