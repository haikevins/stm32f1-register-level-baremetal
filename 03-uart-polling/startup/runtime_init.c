#include <stdint.h>

#include "compiler.h"

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int main(void);

COMPILER_NORETURN void runtime_init(void)
{
    const uint32_t *source = &_sidata;
    uint32_t *destination = &_sdata;

    while (destination < &_edata)
    {
        *destination = *source;
        destination++;
        source++;
    }

    destination = &_sbss;

    while (destination < &_ebss)
    {
        *destination = 0U;
        destination++;
    }

    (void)main();

    for (;;)
    {
    }
}
