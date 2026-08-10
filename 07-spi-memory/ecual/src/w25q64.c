#include "w25q64.h"

#include <stddef.h>

#include "service_config.h"

#define W25Q64_CMD_WRITE_ENABLE    (0x06U)
#define W25Q64_CMD_READ_STATUS1    (0x05U)
#define W25Q64_CMD_READ_DATA       (0x03U)
#define W25Q64_CMD_PAGE_PROGRAM    (0x02U)
#define W25Q64_CMD_SECTOR_ERASE_4K (0x20U)
#define W25Q64_CMD_JEDEC_ID        (0x9FU)

#define W25Q64_STATUS1_BUSY        (0x01U)
#define W25Q64_STATUS1_WEL         (0x02U)

static w25q64_transport_t g_transport;
static bool g_transport_ready;

static bool address_range_valid(uint32_t address, size_t length)
{
    if (length == 0U)
    {
        return address <= W25Q64_SIZE_BYTES;
    }

    if (address >= W25Q64_SIZE_BYTES)
    {
        return false;
    }

    return length <=
        (size_t)(W25Q64_SIZE_BYTES - address);
}

static void encode_address(uint32_t address, uint8_t *bytes)
{
    bytes[0] = (uint8_t)(address >> 16U);
    bytes[1] = (uint8_t)(address >> 8U);
    bytes[2] = (uint8_t)address;
}

static bool command_only(uint8_t command)
{
    bool ok;

    g_transport.select();
    ok = g_transport.transfer(&command, NULL, 1U);
    g_transport.deselect();

    return ok;
}

bool w25q64_read_status1(uint8_t *status1)
{
    const uint8_t command = W25Q64_CMD_READ_STATUS1;
    bool ok;

    if ((!g_transport_ready) || (status1 == NULL))
    {
        return false;
    }

    g_transport.select();

    ok = g_transport.transfer(&command, NULL, 1U);

    if (ok)
    {
        ok = g_transport.transfer(NULL, status1, 1U);
    }

    g_transport.deselect();

    return ok;
}

static bool wait_ready(uint32_t poll_limit)
{
    uint32_t poll;

    for (poll = 0U; poll < poll_limit; poll++)
    {
        uint8_t status1;

        if (!w25q64_read_status1(&status1))
        {
            return false;
        }

        if ((status1 & W25Q64_STATUS1_BUSY) == 0U)
        {
            return true;
        }
    }

    return false;
}

static bool write_enable(void)
{
    uint8_t status1;

    if (!command_only(W25Q64_CMD_WRITE_ENABLE))
    {
        return false;
    }

    if (!w25q64_read_status1(&status1))
    {
        return false;
    }

    return (status1 & W25Q64_STATUS1_WEL) != 0U;
}

bool w25q64_init(const w25q64_transport_t *transport,
                  memory_jedec_id_t *jedec_id)
{
    const uint8_t command = W25Q64_CMD_JEDEC_ID;
    uint8_t id[3] = {0U, 0U, 0U};
    bool ok;

    if ((transport == NULL) ||
        (transport->transfer == NULL) ||
        (transport->select == NULL) ||
        (transport->deselect == NULL) ||
        (jedec_id == NULL))
    {
        return false;
    }

    g_transport = *transport;
    g_transport_ready = true;

    jedec_id->manufacturer_id = 0U;
    jedec_id->memory_type = 0U;
    jedec_id->capacity_id = 0U;

    g_transport.deselect();
    g_transport.select();

    ok = g_transport.transfer(&command, NULL, 1U);

    if (ok)
    {
        ok = g_transport.transfer(NULL, id, sizeof(id));
    }

    g_transport.deselect();

    if (!ok)
    {
        return false;
    }

    jedec_id->manufacturer_id = id[0];
    jedec_id->memory_type = id[1];
    jedec_id->capacity_id = id[2];

    return
        (id[0] == W25Q64_WINBOND_MANUFACTURER_ID) &&
        (id[2] == W25Q64_CAPACITY_ID_64MBIT);
}

bool w25q64_read(uint32_t address,
                 uint8_t *data,
                 size_t length)
{
    uint8_t header[4];
    bool ok;

    if ((!g_transport_ready) ||
        ((data == NULL) && (length != 0U)) ||
        !address_range_valid(address, length))
    {
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    if (!wait_ready(W25Q64_READY_POLL_LIMIT))
    {
        return false;
    }

    header[0] = W25Q64_CMD_READ_DATA;
    encode_address(address, &header[1]);

    g_transport.select();

    ok = g_transport.transfer(header, NULL, sizeof(header));

    if (ok)
    {
        ok = g_transport.transfer(NULL, data, length);
    }

    g_transport.deselect();

    return ok;
}

bool w25q64_page_program(uint32_t address,
                         const uint8_t *data,
                         size_t length)
{
    uint8_t header[4];
    const uint32_t page_offset =
        address % W25Q64_PAGE_SIZE_BYTES;
    bool ok;

    if ((!g_transport_ready) ||
        (data == NULL) ||
        (length == 0U) ||
        (length > W25Q64_PAGE_SIZE_BYTES) ||
        !address_range_valid(address, length) ||
        ((page_offset + length) > W25Q64_PAGE_SIZE_BYTES))
    {
        return false;
    }

    if (!wait_ready(W25Q64_READY_POLL_LIMIT) ||
        !write_enable())
    {
        return false;
    }

    header[0] = W25Q64_CMD_PAGE_PROGRAM;
    encode_address(address, &header[1]);

    g_transport.select();

    ok = g_transport.transfer(header, NULL, sizeof(header));

    if (ok)
    {
        ok = g_transport.transfer(data, NULL, length);
    }

    g_transport.deselect();

    if (!ok)
    {
        return false;
    }

    return wait_ready(W25Q64_PAGE_PROGRAM_POLL_LIMIT);
}

bool w25q64_sector_erase(uint32_t address)
{
    uint8_t command[4];

    if ((!g_transport_ready) ||
        (address >= W25Q64_SIZE_BYTES))
    {
        return false;
    }

    address -= address % W25Q64_SECTOR_SIZE_BYTES;

    if (!wait_ready(W25Q64_READY_POLL_LIMIT) ||
        !write_enable())
    {
        return false;
    }

    command[0] = W25Q64_CMD_SECTOR_ERASE_4K;
    encode_address(address, &command[1]);

    g_transport.select();

    if (!g_transport.transfer(command, NULL, sizeof(command)))
    {
        g_transport.deselect();
        return false;
    }

    g_transport.deselect();

    return wait_ready(W25Q64_SECTOR_ERASE_POLL_LIMIT);
}
