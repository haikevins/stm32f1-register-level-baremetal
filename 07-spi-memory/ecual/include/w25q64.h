#ifndef W25Q64_H
#define W25Q64_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_types.h"

#define W25Q64_SIZE_BYTES        (8388608UL)
#define W25Q64_PAGE_SIZE_BYTES   (256U)
#define W25Q64_SECTOR_SIZE_BYTES (4096UL)

#define W25Q64_WINBOND_MANUFACTURER_ID (0xEFU)
#define W25Q64_CAPACITY_ID_64MBIT       (0x17U)

typedef bool (*w25q64_transfer_fn_t)(
    const uint8_t *transmit_data,
    uint8_t *receive_data,
    size_t length);

typedef void (*w25q64_chip_select_fn_t)(void);

typedef struct
{
    w25q64_transfer_fn_t transfer;
    w25q64_chip_select_fn_t select;
    w25q64_chip_select_fn_t deselect;
} w25q64_transport_t;

bool w25q64_init(const w25q64_transport_t *transport,
                  memory_jedec_id_t *jedec_id);

bool w25q64_read(uint32_t address,
                 uint8_t *data,
                 size_t length);

bool w25q64_page_program(uint32_t address,
                         const uint8_t *data,
                         size_t length);

bool w25q64_sector_erase(uint32_t address);
bool w25q64_read_status1(uint8_t *status1);

#endif
