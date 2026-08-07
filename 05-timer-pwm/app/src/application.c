#include "application.h"

#include <stdbool.h>
#include <stdint.h>

#include "application_config.h"
#include "pwm_service.h"
#include "service_config.h"
#include "time_service.h"

static uint32_t g_last_update_ms;

volatile uint16_t application_pwm_duty_permille;
volatile uint32_t application_pwm_update_count;
volatile bool application_pwm_ramping_up;

void application_init(void)
{
    g_last_update_ms = time_service_now_ms();

    application_pwm_duty_permille = 0U;
    application_pwm_update_count = 0U;
    application_pwm_ramping_up = true;

    pwm_service_set_duty_permille(application_pwm_duty_permille);
}

void application_process(void)
{
    if (!time_service_periodic_due(&g_last_update_ms,
                                   APPLICATION_PWM_UPDATE_PERIOD_MS))
    {
        return;
    }

    uint16_t next_duty = application_pwm_duty_permille;

    if (application_pwm_ramping_up)
    {
        if (((uint32_t)next_duty + APPLICATION_PWM_STEP_PERMILLE) >=
            PWM_SERVICE_DUTY_MAX_PERMILLE)
        {
            next_duty = PWM_SERVICE_DUTY_MAX_PERMILLE;
            application_pwm_ramping_up = false;
        }
        else
        {
            next_duty =
                (uint16_t)((uint32_t)next_duty +
                           APPLICATION_PWM_STEP_PERMILLE);
        }
    }
    else
    {
        if (next_duty <= APPLICATION_PWM_STEP_PERMILLE)
        {
            next_duty = 0U;
            application_pwm_ramping_up = true;
        }
        else
        {
            next_duty =
                (uint16_t)(next_duty - APPLICATION_PWM_STEP_PERMILLE);
        }
    }

    application_pwm_duty_permille = next_duty;
    application_pwm_update_count++;

    pwm_service_set_duty_permille(next_duty);
}
