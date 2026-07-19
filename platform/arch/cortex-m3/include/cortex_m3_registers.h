#ifndef CORTEX_M3_REGISTERS_H
#define CORTEX_M3_REGISTERS_H

#include <stdint.h>

#define CORTEX_M3_SCB_BASE (UINT32_C(0xE000ED00))

typedef struct
{
    volatile const uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint8_t SHP[12];
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
    volatile uint32_t AFSR;
} cortex_m3_scb_registers_t;

#define CORTEX_M3_SCB ((cortex_m3_scb_registers_t *)CORTEX_M3_SCB_BASE)

#define CORTEX_M3_SCB_AIRCR_VECTKEY     (UINT32_C(0x5FA) << 16U)
#define CORTEX_M3_SCB_AIRCR_SYSRESETREQ (UINT32_C(1) << 2U)

#endif
