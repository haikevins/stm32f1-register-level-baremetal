#ifndef MEMORY_SERVICE_H
#define MEMORY_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_types.h"

bool memory_service_init(void);
memory_jedec_id_t memory_service_get_jedec_id(void);

bool memory_service_read(uint32_t address,
                         uint8_t *data,
                         size_t length);

bool memory_service_program_page(uint32_t address,
                                 const uint8_t *data,
                                 size_t length);

bool memory_service_erase_sector(uint32_t address);

#endif
