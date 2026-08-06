#include <stddef.h>

#include "stm32f103xb.h"

_Static_assert(offsetof(stm32_afio_registers_t, EXTICR) == 0x08U,
               "AFIO EXTICR register offset is invalid");
_Static_assert(offsetof(stm32_exti_registers_t, PR) == 0x14U,
               "EXTI PR register offset is invalid");
_Static_assert(offsetof(stm32_gpio_registers_t, CRH) == 0x04U,
               "GPIO CRH register offset is invalid");
_Static_assert(offsetof(stm32_gpio_registers_t, BSRR) == 0x10U,
               "GPIO BSRR register offset is invalid");
_Static_assert(offsetof(stm32_rcc_registers_t, APB2ENR) == 0x18U,
               "RCC APB2ENR register offset is invalid");
_Static_assert(offsetof(stm32_flash_registers_t, ACR) == 0x00U,
               "FLASH ACR register offset is invalid");
