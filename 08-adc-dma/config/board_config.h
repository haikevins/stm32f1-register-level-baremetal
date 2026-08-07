#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_HSE_FREQUENCY_HZ                   (8000000UL)
#define BOARD_TARGET_CLOCK_HZ                     (72000000UL)

#define BOARD_ADC_REFERENCE_MV                    (3300UL)
#define BOARD_ADC_MAX_RAW_VALUE                   (4095UL)
#define BOARD_ADC_MAX_CLOCK_HZ                    (12000000UL)
#define BOARD_ADC_SAMPLE_RATE_HZ                  (1000UL)
#define BOARD_ADC_TRIGGER_TIMER_TICK_HZ           (1000000UL)

#define BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT         (64U)
#define BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT          \
    (BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT / 2U)

#define BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS  (1000000UL)
#define BOARD_ADC_DMA_IRQ_PRIORITY                (1U)

#if BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT < 2U
#error "BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT must be at least two."
#endif

#if (BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT % 2U) != 0U
#error "BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT must be even."
#endif

#if BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT > 65535U
#error "BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT must fit DMA CNDTR."
#endif

#if BOARD_ADC_SAMPLE_RATE_HZ == 0UL
#error "BOARD_ADC_SAMPLE_RATE_HZ must be greater than zero."
#endif

#if BOARD_ADC_TRIGGER_TIMER_TICK_HZ == 0UL
#error "BOARD_ADC_TRIGGER_TIMER_TICK_HZ must be greater than zero."
#endif

#if BOARD_ADC_MAX_CLOCK_HZ > 14000000UL
#error "STM32F103 ADC clock must not exceed 14 MHz."
#endif

#endif
