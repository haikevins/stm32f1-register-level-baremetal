#ifndef BOARD_PWM_H
#define BOARD_PWM_H

#include <stdbool.h>
#include <stdint.h>

bool board_pwm_init(uint32_t timer_clock_hz);
void board_pwm_set_duty_permille(uint16_t duty_permille);

#endif
