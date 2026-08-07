#ifndef CORTEX_M3_H
#define CORTEX_M3_H

#include <stdint.h>

static inline void cortex_m3_enable_irq(void)
{
    __asm volatile("cpsie i" ::: "memory");
}

static inline void cortex_m3_disable_irq(void)
{
    __asm volatile("cpsid i" ::: "memory");
}

static inline uint32_t cortex_m3_get_primask(void)
{
    uint32_t primask;
    __asm volatile("mrs %0, primask" : "=r"(primask) :: "memory");
    return primask;
}

static inline void cortex_m3_wait_for_interrupt(void)
{
    __asm volatile("wfi" ::: "memory");
}

static inline void cortex_m3_data_sync_barrier(void)
{
    __asm volatile("dsb" ::: "memory");
}

static inline void cortex_m3_instruction_sync_barrier(void)
{
    __asm volatile("isb" ::: "memory");
}

static inline void cortex_m3_nop(void)
{
    __asm volatile("nop");
}

void cortex_m3_system_reset(void);

#endif
