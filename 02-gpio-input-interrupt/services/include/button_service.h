#ifndef BUTTON_SERVICE_H
#define BUTTON_SERVICE_H

#include <stdbool.h>

void button_service_init(void);
bool button_service_take_press(void);

#endif
