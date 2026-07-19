#include "event_service.h"

#include "service_config.h"
#include "static_queue.h"

static service_event_t g_event_storage[SERVICE_EVENT_QUEUE_CAPACITY];
static static_queue_t g_event_queue;

bool event_service_init(void)
{
    return static_queue_init(&g_event_queue,
                             g_event_storage,
                             sizeof(service_event_t),
                             SERVICE_EVENT_QUEUE_CAPACITY);
}

bool event_service_publish(const service_event_t *event)
{
    return static_queue_push(&g_event_queue, event);
}

bool event_service_get(service_event_t *event)
{
    return static_queue_pop(&g_event_queue, event);
}
