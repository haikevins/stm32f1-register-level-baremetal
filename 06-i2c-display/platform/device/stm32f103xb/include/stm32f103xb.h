#ifndef STM32F103XB_H
#define STM32F103XB_H

#include <stdint.h>

#include "stm32f103xb_memory.h"

typedef struct
{
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile const uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} stm32_gpio_registers_t;

typedef struct
{
    volatile const uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} stm32_usart_registers_t;



typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile const uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
} stm32_i2c_registers_t;

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    uint32_t RESERVED0;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    uint32_t RESERVED1;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} stm32_timer_registers_t;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} stm32_rcc_registers_t;

typedef struct
{
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t AR;
    volatile uint32_t RESERVED;
    volatile uint32_t OBR;
    volatile uint32_t WRPR;
} stm32_flash_registers_t;

#define STM32_GPIOA  ((stm32_gpio_registers_t *)STM32_GPIOA_BASE)
#define STM32_GPIOB  ((stm32_gpio_registers_t *)STM32_GPIOB_BASE)
#define STM32_GPIOC  ((stm32_gpio_registers_t *)STM32_GPIOC_BASE)
#define STM32_GPIOD  ((stm32_gpio_registers_t *)STM32_GPIOD_BASE)
#define STM32_GPIOE  ((stm32_gpio_registers_t *)STM32_GPIOE_BASE)
#define STM32_USART1 ((stm32_usart_registers_t *)STM32_USART1_BASE)
#define STM32_TIM2   ((stm32_timer_registers_t *)STM32_TIM2_BASE)
#define STM32_I2C1   ((stm32_i2c_registers_t *)STM32_I2C1_BASE)

#define STM32_RCC     ((stm32_rcc_registers_t *)STM32_RCC_BASE)
#define STM32_FLASH   ((stm32_flash_registers_t *)STM32_FLASH_REG_BASE)

#endif
