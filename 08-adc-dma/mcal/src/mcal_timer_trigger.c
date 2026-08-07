#include "mcal_timer_trigger.h"

#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

bool mcal_timer3_trigger_init(uint32_t timer_clock_hz,
                              uint32_t timer_tick_hz,
                              uint32_t trigger_frequency_hz)
{
    uint32_t prescaler_divider;
    uint32_t period_counts;

    if ((timer_clock_hz == 0U) ||
        (timer_tick_hz == 0U) ||
        (trigger_frequency_hz == 0U) ||
        ((timer_clock_hz % timer_tick_hz) != 0U) ||
        ((timer_tick_hz % trigger_frequency_hz) != 0U))
    {
        return false;
    }

    prescaler_divider = timer_clock_hz / timer_tick_hz;
    period_counts = timer_tick_hz / trigger_frequency_hz;

    if ((prescaler_divider == 0U) ||
        (prescaler_divider > UINT32_C(0x10000)) ||
        (period_counts == 0U) ||
        (period_counts > UINT32_C(0x10000)))
    {
        return false;
    }

    STM32_RCC->APB1ENR |= STM32_RCC_APB1ENR_TIM3EN;
    (void)STM32_RCC->APB1ENR;

    STM32_TIM3->CR1 = 0U;
    STM32_TIM3->CR2 = 0U;
    STM32_TIM3->SMCR = 0U;
    STM32_TIM3->DIER = 0U;
    STM32_TIM3->SR = 0U;
    STM32_TIM3->CNT = 0U;
    STM32_TIM3->PSC = prescaler_divider - 1U;
    STM32_TIM3->ARR = period_counts - 1U;

    STM32_TIM3->CR2 =
        STM32_TIM_CR2_MMS_UPDATE;

    STM32_TIM3->EGR = STM32_TIM_EGR_UG;
    STM32_TIM3->SR = 0U;

    return true;
}

void mcal_timer3_trigger_start(void)
{
    STM32_TIM3->CR1 |= STM32_TIM_CR1_CEN;
}

void mcal_timer3_trigger_stop(void)
{
    STM32_TIM3->CR1 &= ~STM32_TIM_CR1_CEN;
}
