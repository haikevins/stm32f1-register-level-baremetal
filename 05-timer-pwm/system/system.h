#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>

bool system_init(void);
void system_idle(void);
void system_panic(void);

#endif
