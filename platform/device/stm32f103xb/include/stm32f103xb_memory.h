#ifndef STM32F103XB_MEMORY_H
#define STM32F103XB_MEMORY_H

#include <stdint.h>

#define STM32_FLASH_MEMORY_BASE (UINT32_C(0x08000000))
#define STM32_SRAM_BASE         (UINT32_C(0x20000000))

#define STM32_PERIPH_BASE       (UINT32_C(0x40000000))
#define STM32_APB1_BASE         STM32_PERIPH_BASE
#define STM32_APB2_BASE         (UINT32_C(0x40010000))
#define STM32_AHB_BASE          (UINT32_C(0x40018000))

/* Add peripheral base addresses from RM0008 as MCAL modules are introduced. */

#endif
