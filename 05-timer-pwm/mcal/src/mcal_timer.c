#include "mcal_timer.h"

#include <stddef.h>

#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

#define PWM_DUTY_MAX_PERMILLE (1000U)

static stm32_timer_registers_t *timer_registers(mcal_timer_instance_t instance)
{
    switch (instance)
    {
        case MCAL_TIMER_INSTANCE_2:
            return STM32_TIM2;
        default:
            return NULL;
    }
}

static uint32_t timer_clock_mask(mcal_timer_instance_t instance)
{
    switch (instance)
    {
        case MCAL_TIMER_INSTANCE_2:
            return STM32_RCC_APB1ENR_TIM2EN;
        default:
            return 0U;
    }
}

static void set_channel_1_duty(stm32_timer_registers_t *timer,
                               uint16_t duty_permille)
{
    uint32_t duty = duty_permille;

    if (duty > PWM_DUTY_MAX_PERMILLE)
    {
        duty = PWM_DUTY_MAX_PERMILLE;
    }

    /*
     * ARR stores period_counts - 1. CCR1 may equal period_counts to produce
     * a true 100% duty cycle in PWM mode 1 (CNT is always less than CCR1).
     */
    const uint32_t period_counts = timer->ARR + 1U;
    timer->CCR1 =
        (period_counts * duty) / PWM_DUTY_MAX_PERMILLE;
}

bool mcal_timer_pwm_init(mcal_timer_instance_t instance,
                         mcal_timer_channel_t channel,
                         uint32_t timer_clock_hz,
                         uint32_t timer_tick_hz,
                         uint32_t pwm_frequency_hz,
                         uint16_t initial_duty_permille)
{
    stm32_timer_registers_t *timer = timer_registers(instance);
    const uint32_t clock_mask = timer_clock_mask(instance);

    if ((timer == NULL) ||
        (channel != MCAL_TIMER_CHANNEL_1) ||
        (clock_mask == 0U) ||
        (timer_clock_hz == 0U) ||
        (timer_tick_hz == 0U) ||
        (pwm_frequency_hz == 0U) ||
        ((timer_clock_hz % timer_tick_hz) != 0U) ||
        ((timer_tick_hz % pwm_frequency_hz) != 0U) ||
        (initial_duty_permille > PWM_DUTY_MAX_PERMILLE))
    {
        return false;
    }

    const uint32_t prescaler_divider = timer_clock_hz / timer_tick_hz;
    const uint32_t period_counts = timer_tick_hz / pwm_frequency_hz;

    if ((prescaler_divider == 0U) ||
        (prescaler_divider > UINT32_C(0x10000)) ||
        (period_counts == 0U) ||
        (period_counts > UINT32_C(0x10000)))
    {
        return false;
    }

    STM32_RCC->APB1ENR |= clock_mask;
    (void)STM32_RCC->APB1ENR;

    timer->CR1 = 0U;
    timer->CR2 = 0U;
    timer->SMCR = 0U;
    timer->DIER = 0U;
    timer->CCER = 0U;
    timer->CCMR1 = 0U;

    timer->PSC = prescaler_divider - 1U;
    timer->ARR = period_counts - 1U;
    timer->CNT = 0U;

    /*
     * Channel 1 output compare:
     * OC1M = 110b -> PWM mode 1
     * OC1PE = 1   -> preload CCR1 and latch it on update events
     */
    timer->CCMR1 =
        STM32_TIM_CCMR1_OC1M_PWM1 |
        STM32_TIM_CCMR1_OC1PE;

    set_channel_1_duty(timer, initial_duty_permille);

    timer->CCER = STM32_TIM_CCER_CC1E;
    timer->CR1 = STM32_TIM_CR1_ARPE;

    /*
     * UG loads PSC/ARR/CCR1 shadow values before the counter starts.
     */
    timer->EGR = STM32_TIM_EGR_UG;
    timer->SR = 0U;
    timer->CR1 |= STM32_TIM_CR1_CEN;

    return true;
}

void mcal_timer_pwm_set_duty_permille(mcal_timer_instance_t instance,
                                      mcal_timer_channel_t channel,
                                      uint16_t duty_permille)
{
    stm32_timer_registers_t *timer = timer_registers(instance);

    if ((timer == NULL) || (channel != MCAL_TIMER_CHANNEL_1))
    {
        return;
    }

    set_channel_1_duty(timer, duty_permille);
}
