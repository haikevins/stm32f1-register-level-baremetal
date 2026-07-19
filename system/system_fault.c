#include "system.h"

void NMI_Handler(void)
{
    system_panic();
}

void HardFault_Handler(void)
{
    system_panic();
}

void MemManage_Handler(void)
{
    system_panic();
}

void BusFault_Handler(void)
{
    system_panic();
}

void UsageFault_Handler(void)
{
    system_panic();
}
