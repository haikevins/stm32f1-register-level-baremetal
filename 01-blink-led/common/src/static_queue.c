#include "static_queue.h"

static void copy_bytes(uint8_t *destination, const uint8_t *source, size_t length)
{
    for (size_t index = 0U; index < length; ++index)
    {
        destination[index] = source[index];
    }
}

bool static_queue_init(static_queue_t *queue,
                       void *storage,
                       size_t item_size,
                       size_t capacity)
{
    if ((queue == NULL) || (storage == NULL) || (item_size == 0U) || (capacity == 0U))
    {
        return false;
    }

    queue->storage = (uint8_t *)storage;
    queue->item_size = item_size;
    queue->capacity = capacity;
    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;

    return true;
}

bool static_queue_push(static_queue_t *queue, const void *item)
{
    if ((queue == NULL) || (item == NULL) || (queue->count >= queue->capacity))
    {
        return false;
    }

    uint8_t *destination = &queue->storage[queue->head * queue->item_size];
    copy_bytes(destination, (const uint8_t *)item, queue->item_size);

    queue->head = (queue->head + 1U) % queue->capacity;
    queue->count++;

    return true;
}

bool static_queue_pop(static_queue_t *queue, void *item)
{
    if ((queue == NULL) || (item == NULL) || (queue->count == 0U))
    {
        return false;
    }

    const uint8_t *source = &queue->storage[queue->tail * queue->item_size];
    copy_bytes((uint8_t *)item, source, queue->item_size);

    queue->tail = (queue->tail + 1U) % queue->capacity;
    queue->count--;

    return true;
}

bool static_queue_is_empty(const static_queue_t *queue)
{
    return (queue == NULL) || (queue->count == 0U);
}

size_t static_queue_count(const static_queue_t *queue)
{
    return (queue == NULL) ? 0U : queue->count;
}
