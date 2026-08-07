#include "display_service.h"

#include "board_config.h"
#include "board_display_bus.h"
#include "ssd1306.h"

bool display_service_init(void)
{
    static const ssd1306_transport_t transport =
    {
        .write = board_display_bus_write,
        .delay_ms = board_display_bus_delay_ms,
        .power_on_delay_ms = BOARD_DISPLAY_POWER_ON_DELAY_MS
    };

    return ssd1306_init(&transport);
}

void display_service_clear(void)
{
    ssd1306_clear();
}

void display_service_draw_text(uint8_t x, uint8_t y, const char *text)
{
    ssd1306_draw_text(x, y, text);
}

void display_service_draw_progress_bar(uint8_t x,
                                       uint8_t y,
                                       uint8_t width,
                                       uint8_t height,
                                       uint8_t percent)
{
    ssd1306_draw_progress_bar(x, y, width, height, percent);
}

bool display_service_present(void)
{
    return ssd1306_update();
}
