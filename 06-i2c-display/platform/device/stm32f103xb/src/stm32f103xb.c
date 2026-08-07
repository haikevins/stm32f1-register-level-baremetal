#include <stddef.h>

#include "stm32f103xb.h"

_Static_assert(offsetof(stm32_gpio_registers_t, CRH) == 0x04U,
               "GPIO CRH register offset is invalid");
_Static_assert(offsetof(stm32_gpio_registers_t, BSRR) == 0x10U,
               "GPIO BSRR register offset is invalid");
_Static_assert(offsetof(stm32_usart_registers_t, BRR) == 0x08U,
               "USART BRR register offset is invalid");
_Static_assert(offsetof(stm32_usart_registers_t, CR1) == 0x0CU,
               "USART CR1 register offset is invalid");
_Static_assert(offsetof(stm32_timer_registers_t, ARR) == 0x2CU,
               "TIM ARR register offset is invalid");
_Static_assert(offsetof(stm32_timer_registers_t, CCR1) == 0x34U,
               "TIM CCR1 register offset is invalid");
_Static_assert(offsetof(stm32_rcc_registers_t, APB2ENR) == 0x18U,
               "RCC APB2ENR register offset is invalid");
_Static_assert(offsetof(stm32_rcc_registers_t, APB1ENR) == 0x1CU,
               "RCC APB1ENR register offset is invalid");
_Static_assert(offsetof(stm32_flash_registers_t, ACR) == 0x00U,
               "FLASH ACR register offset is invalid");
