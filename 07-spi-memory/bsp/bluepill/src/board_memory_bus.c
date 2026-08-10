#include "board_memory_bus.h"

#include "board_config.h"
#include "board_pins.h"
#include "mcal_delay.h"
#include "mcal_gpio.h"
#include "mcal_rcc.h"
#include "mcal_spi.h"

bool board_memory_bus_init(uint32_t peripheral_clock_hz)
{
    /*
     * Drive CS high before making it an output so the memory remains
     * deselected during GPIO/SPI configuration.
     */
    if (!mcal_gpio_configure(BOARD_MEMORY_SPI_PORT,
                             BOARD_MEMORY_CS_PIN,
                             MCAL_GPIO_MODE_OUTPUT_PP_50MHZ,
                             MCAL_GPIO_LEVEL_HIGH))
    {
        return false;
    }

    if (!mcal_gpio_configure(BOARD_MEMORY_SPI_PORT,
                             BOARD_MEMORY_SCK_PIN,
                             MCAL_GPIO_MODE_AF_PP_50MHZ,
                             MCAL_GPIO_LEVEL_LOW))
    {
        return false;
    }

    if (!mcal_gpio_configure(BOARD_MEMORY_SPI_PORT,
                             BOARD_MEMORY_MOSI_PIN,
                             MCAL_GPIO_MODE_AF_PP_50MHZ,
                             MCAL_GPIO_LEVEL_LOW))
    {
        return false;
    }

    if (!mcal_gpio_configure(BOARD_MEMORY_SPI_PORT,
                             BOARD_MEMORY_MISO_PIN,
                             MCAL_GPIO_MODE_INPUT_FLOATING,
                             MCAL_GPIO_LEVEL_LOW))
    {
        return false;
    }

    board_memory_bus_deselect();

    return mcal_spi_init(MCAL_SPI_INSTANCE_1,
                         peripheral_clock_hz,
                         BOARD_MEMORY_SPI_MAX_HZ,
                         MCAL_SPI_MODE_0);
}

bool board_memory_bus_transfer(const uint8_t *transmit_data,
                               uint8_t *receive_data,
                               size_t length)
{
    return mcal_spi_transfer(MCAL_SPI_INSTANCE_1,
                             transmit_data,
                             receive_data,
                             length);
}

void board_memory_bus_select(void)
{
    mcal_gpio_write(BOARD_MEMORY_SPI_PORT,
                    BOARD_MEMORY_CS_PIN,
                    MCAL_GPIO_LEVEL_LOW);
}

void board_memory_bus_deselect(void)
{
    mcal_gpio_write(BOARD_MEMORY_SPI_PORT,
                    BOARD_MEMORY_CS_PIN,
                    MCAL_GPIO_LEVEL_HIGH);
}

void board_memory_bus_delay_ms(uint32_t delay_ms)
{
    mcal_delay_busy_ms(mcal_rcc_get_system_clock_hz(), delay_ms);
}
