#include "board_pwm.h"

#include "board_config.h"
#include "board_pins.h"
#include "mcal_gpio.h"
#include "mcal_timer.h"

bool board_pwm_init(uint32_t timer_clock_hz)
{
    if (!mcal_gpio_configure(BOARD_PWM_PORT,
                             BOARD_PWM_PIN,
                             MCAL_GPIO_MODE_AF_PP_50MHZ,
                             MCAL_GPIO_LEVEL_LOW))
    {
        return false;
    }

    return mcal_timer_pwm_init(MCAL_TIMER_INSTANCE_2,
                               MCAL_TIMER_CHANNEL_1,
                               timer_clock_hz,
                               BOARD_PWM_TIMER_TICK_HZ,
                               BOARD_PWM_FREQUENCY_HZ,
                               0U);
}

void board_pwm_set_duty_permille(uint16_t duty_permille)
{
    mcal_timer_pwm_set_duty_permille(MCAL_TIMER_INSTANCE_2,
                                     MCAL_TIMER_CHANNEL_1,
                                     duty_permille);
}
