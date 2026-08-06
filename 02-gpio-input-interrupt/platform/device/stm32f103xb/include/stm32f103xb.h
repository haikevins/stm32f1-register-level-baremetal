#ifndef STM32F103XB_H
#define STM32F103XB_H

#include <stdint.h>

#include "stm32f103xb_memory.h"

typedef struct
{
    volatile uint32_t EVCR;
    volatile uint32_t MAPR;
    volatile uint32_t EXTICR[4];
    uint32_t RESERVED;
    volatile uint32_t MAPR2;
} stm32_afio_registers_t;

typedef struct
{
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} stm32_exti_registers_t;

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

#define STM32_AFIO  ((stm32_afio_registers_t *)STM32_AFIO_BASE)
#define STM32_EXTI  ((stm32_exti_registers_t *)STM32_EXTI_BASE)
#define STM32_GPIOA ((stm32_gpio_registers_t *)STM32_GPIOA_BASE)
#define STM32_GPIOB ((stm32_gpio_registers_t *)STM32_GPIOB_BASE)
#define STM32_GPIOC ((stm32_gpio_registers_t *)STM32_GPIOC_BASE)
#define STM32_GPIOD ((stm32_gpio_registers_t *)STM32_GPIOD_BASE)
#define STM32_GPIOE ((stm32_gpio_registers_t *)STM32_GPIOE_BASE)

#define STM32_RCC   ((stm32_rcc_registers_t *)STM32_RCC_BASE)
#define STM32_FLASH ((stm32_flash_registers_t *)STM32_FLASH_REG_BASE)

#endif
