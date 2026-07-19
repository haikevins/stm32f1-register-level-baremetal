#ifndef EVENT_SERVICE_H
#define EVENT_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t id;
    uint16_t source;
    uint32_t data;
} service_event_t;

bool event_service_init(void);
bool event_service_publish(const service_event_t *event);
bool event_service_get(service_event_t *event);

#endif
