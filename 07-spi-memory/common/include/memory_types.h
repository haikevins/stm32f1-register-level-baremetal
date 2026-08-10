#ifndef MEMORY_TYPES_H
#define MEMORY_TYPES_H

#include <stdint.h>

typedef struct
{
    uint8_t manufacturer_id;
    uint8_t memory_type;
    uint8_t capacity_id;
} memory_jedec_id_t;

#endif
