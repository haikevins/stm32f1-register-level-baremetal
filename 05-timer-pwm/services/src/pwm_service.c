#include "pwm_service.h"

#include "board_pwm.h"
#include "service_config.h"

void pwm_service_init(void)
{
    board_pwm_set_duty_permille(0U);
}

void pwm_service_set_duty_permille(uint16_t duty_permille)
{
    if (duty_permille > PWM_SERVICE_DUTY_MAX_PERMILLE)
    {
        duty_permille = PWM_SERVICE_DUTY_MAX_PERMILLE;
    }

    board_pwm_set_duty_permille(duty_permille);
}
