#include "mcal_adc.h"

#include "cortex_m3.h"
#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

#define ADC_CHANNEL_COUNT (16U)

static bool configure_adc_clock(uint32_t apb2_clock_hz,
                                uint32_t max_adc_clock_hz)
{
    static const uint8_t divisors[] = {2U, 4U, 6U, 8U};
    static const uint32_t encodings[] =
    {
        STM32_RCC_CFGR_ADCPRE_DIV2,
        STM32_RCC_CFGR_ADCPRE_DIV4,
        STM32_RCC_CFGR_ADCPRE_DIV6,
        STM32_RCC_CFGR_ADCPRE_DIV8
    };
    uint32_t index;

    if ((apb2_clock_hz == 0U) || (max_adc_clock_hz == 0U))
    {
        return false;
    }

    for (index = 0U;
         index < (sizeof(divisors) / sizeof(divisors[0]));
         index++)
    {
        const uint32_t adc_clock_hz =
            apb2_clock_hz / divisors[index];

        if (adc_clock_hz <= max_adc_clock_hz)
        {
            STM32_RCC->CFGR =
                (STM32_RCC->CFGR & ~STM32_RCC_CFGR_ADCPRE_MASK) |
                encodings[index];
            return true;
        }
    }

    return false;
}

static bool wait_until_clear(volatile const uint32_t *reg,
                             uint32_t mask,
                             uint32_t timeout_iterations)
{
    while (timeout_iterations > 0U)
    {
        if ((*reg & mask) == 0U)
        {
            return true;
        }

        timeout_iterations--;
    }

    return false;
}

static void configure_sample_time(uint8_t channel)
{
    const uint32_t sample_code = STM32_ADC_SAMPLE_55_CYCLES_5;

    if (channel <= 9U)
    {
        const uint32_t shift = (uint32_t)channel * 3U;
        const uint32_t mask = UINT32_C(0x7) << shift;

        STM32_ADC1->SMPR2 =
            (STM32_ADC1->SMPR2 & ~mask) |
            (sample_code << shift);
    }
    else
    {
        const uint32_t shift = ((uint32_t)channel - 10U) * 3U;
        const uint32_t mask = UINT32_C(0x7) << shift;

        STM32_ADC1->SMPR1 =
            (STM32_ADC1->SMPR1 & ~mask) |
            (sample_code << shift);
    }
}

bool mcal_adc1_init_regular_channel(
    uint8_t channel,
    uint32_t apb2_clock_hz,
    uint32_t max_adc_clock_hz,
    mcal_adc_trigger_t trigger,
    uint32_t calibration_timeout_iterations)
{
    uint32_t stabilization_count;

    if ((channel >= ADC_CHANNEL_COUNT) ||
        (trigger != MCAL_ADC_TRIGGER_TIM3_TRGO) ||
        (calibration_timeout_iterations == 0U) ||
        !configure_adc_clock(apb2_clock_hz, max_adc_clock_hz))
    {
        return false;
    }

    STM32_RCC->APB2ENR |= STM32_RCC_APB2ENR_ADC1EN;
    (void)STM32_RCC->APB2ENR;

    STM32_RCC->APB2RSTR |= STM32_RCC_APB2RSTR_ADC1RST;
    STM32_RCC->APB2RSTR &= ~STM32_RCC_APB2RSTR_ADC1RST;

    STM32_ADC1->CR1 = 0U;
    STM32_ADC1->CR2 = 0U;
    STM32_ADC1->SMPR1 = 0U;
    STM32_ADC1->SMPR2 = 0U;
    STM32_ADC1->SQR1 = 0U;
    STM32_ADC1->SQR2 = 0U;
    STM32_ADC1->SQR3 = (uint32_t)channel;

    configure_sample_time(channel);

    /*
     * ADON powers the ADC. Wait well beyond the minimum stabilization time
     * before calibration; exact timing is not critical during initialization.
     */
    STM32_ADC1->CR2 = STM32_ADC_CR2_ADON;

    for (stabilization_count = 0U;
         stabilization_count < 1000U;
         stabilization_count++)
    {
        cortex_m3_nop();
    }

    STM32_ADC1->CR2 |= STM32_ADC_CR2_RSTCAL;

    if (!wait_until_clear(&STM32_ADC1->CR2,
                          STM32_ADC_CR2_RSTCAL,
                          calibration_timeout_iterations))
    {
        return false;
    }

    STM32_ADC1->CR2 |= STM32_ADC_CR2_CAL;

    if (!wait_until_clear(&STM32_ADC1->CR2,
                          STM32_ADC_CR2_CAL,
                          calibration_timeout_iterations))
    {
        return false;
    }

    STM32_ADC1->CR2 =
        STM32_ADC_CR2_ADON |
        STM32_ADC_CR2_DMA |
        STM32_ADC_CR2_EXTSEL_TIM3_TRGO |
        STM32_ADC_CR2_EXTTRIG;

    return true;
}

uintptr_t mcal_adc1_data_register_address(void)
{
    return (uintptr_t)&STM32_ADC1->DR;
}
