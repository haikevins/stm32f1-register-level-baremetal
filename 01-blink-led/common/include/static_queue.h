#ifndef STATIC_QUEUE_H
#define STATIC_QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *storage;
    size_t item_size;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} static_queue_t;

bool static_queue_init(static_queue_t *queue,
                       void *storage,
                       size_t item_size,
                       size_t capacity);
bool static_queue_push(static_queue_t *queue, const void *item);
bool static_queue_pop(static_queue_t *queue, void *item);
bool static_queue_is_empty(const static_queue_t *queue);
size_t static_queue_count(const static_queue_t *queue);

#endif
