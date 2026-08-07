#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SSD1306_WIDTH  (128U)
#define SSD1306_HEIGHT (64U)

typedef bool (*ssd1306_write_fn_t)(bool data_mode,
                                   const uint8_t *data,
                                   size_t length);
typedef void (*ssd1306_delay_ms_fn_t)(uint32_t delay_ms);

typedef struct
{
    ssd1306_write_fn_t write;
    ssd1306_delay_ms_fn_t delay_ms;
    uint32_t power_on_delay_ms;
} ssd1306_transport_t;

bool ssd1306_init(const ssd1306_transport_t *transport);
void ssd1306_clear(void);
void ssd1306_draw_pixel(uint8_t x, uint8_t y, bool illuminated);
void ssd1306_draw_text(uint8_t x, uint8_t y, const char *text);
void ssd1306_draw_progress_bar(uint8_t x,
                               uint8_t y,
                               uint8_t width,
                               uint8_t height,
                               uint8_t percent);
bool ssd1306_update(void);

#endif
