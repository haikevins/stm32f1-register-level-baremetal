#ifndef PWM_SERVICE_H
#define PWM_SERVICE_H

#include <stdint.h>

void pwm_service_init(void);
void pwm_service_set_duty_permille(uint16_t duty_permille);

#endif
