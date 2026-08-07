#include "ssd1306.h"

#include <stddef.h>
#include <stdint.h>


#define SSD1306_PAGE_COUNT       (SSD1306_HEIGHT / 8U)
#define SSD1306_FRAMEBUFFER_SIZE (SSD1306_WIDTH * SSD1306_PAGE_COUNT)
#define SSD1306_GLYPH_WIDTH      (5U)
#define SSD1306_GLYPH_HEIGHT     (7U)
#define SSD1306_GLYPH_ADVANCE    (6U)

typedef struct
{
    char character;
    uint8_t columns[SSD1306_GLYPH_WIDTH];
} ssd1306_glyph_t;

static uint8_t s_framebuffer[SSD1306_FRAMEBUFFER_SIZE];
static ssd1306_transport_t s_transport;

static const ssd1306_glyph_t s_glyphs[] =
{
    {' ', {0x00U, 0x00U, 0x00U, 0x00U, 0x00U}},
    {'-', {0x08U, 0x08U, 0x08U, 0x08U, 0x08U}},
    {':', {0x00U, 0x00U, 0x36U, 0x00U, 0x00U}},
    {'.', {0x00U, 0x00U, 0x60U, 0x00U, 0x00U}},
    {'0', {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU}},
    {'1', {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U}},
    {'2', {0x42U, 0x61U, 0x51U, 0x49U, 0x46U}},
    {'3', {0x41U, 0x49U, 0x49U, 0x49U, 0x36U}},
    {'4', {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U}},
    {'5', {0x4FU, 0x49U, 0x49U, 0x49U, 0x31U}},
    {'6', {0x3EU, 0x49U, 0x49U, 0x49U, 0x30U}},
    {'7', {0x01U, 0x71U, 0x09U, 0x05U, 0x03U}},
    {'8', {0x36U, 0x49U, 0x49U, 0x49U, 0x36U}},
    {'9', {0x06U, 0x49U, 0x49U, 0x49U, 0x3EU}},
    {'A', {0x7EU, 0x09U, 0x09U, 0x09U, 0x7EU}},
    {'B', {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U}},
    {'C', {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U}},
    {'D', {0x7FU, 0x41U, 0x41U, 0x41U, 0x3EU}},
    {'E', {0x7FU, 0x49U, 0x49U, 0x49U, 0x41U}},
    {'F', {0x7FU, 0x09U, 0x09U, 0x09U, 0x01U}},
    {'G', {0x3EU, 0x41U, 0x49U, 0x49U, 0x3AU}},
    {'H', {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU}},
    {'I', {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U}},
    {'J', {0x30U, 0x40U, 0x41U, 0x3FU, 0x01U}},
    {'K', {0x7FU, 0x08U, 0x14U, 0x22U, 0x41U}},
    {'L', {0x7FU, 0x40U, 0x40U, 0x40U, 0x40U}},
    {'M', {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU}},
    {'N', {0x7FU, 0x02U, 0x04U, 0x08U, 0x7FU}},
    {'O', {0x3EU, 0x41U, 0x41U, 0x41U, 0x3EU}},
    {'P', {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U}},
    {'Q', {0x3EU, 0x41U, 0x51U, 0x21U, 0x5EU}},
    {'R', {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U}},
    {'S', {0x46U, 0x49U, 0x49U, 0x49U, 0x31U}},
    {'T', {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U}},
    {'U', {0x3FU, 0x40U, 0x40U, 0x40U, 0x3FU}},
    {'V', {0x1FU, 0x20U, 0x40U, 0x20U, 0x1FU}},
    {'W', {0x3FU, 0x40U, 0x38U, 0x40U, 0x3FU}},
    {'X', {0x63U, 0x14U, 0x08U, 0x14U, 0x63U}},
    {'Y', {0x03U, 0x04U, 0x78U, 0x04U, 0x03U}},
    {'Z', {0x61U, 0x51U, 0x49U, 0x45U, 0x43U}},
};

static bool ssd1306_write(
    bool data_mode,
    const uint8_t *bytes,
    size_t length)
{
    if (s_transport.write == NULL)
    {
        return false;
    }

    return s_transport.write(data_mode, bytes, length);
}

static bool ssd1306_write_commands(
    const uint8_t *commands,
    size_t length)
{
    return ssd1306_write(false, commands, length);
}

static bool ssd1306_write_data(
    const uint8_t *data,
    size_t length)
{
    return ssd1306_write(true, data, length);
}

static const uint8_t *ssd1306_find_glyph(char character)
{
    size_t index;

    for (index = 0U;
         index < (sizeof(s_glyphs) / sizeof(s_glyphs[0]));
         index++)
    {
        if (s_glyphs[index].character == character)
        {
            return s_glyphs[index].columns;
        }
    }

    return s_glyphs[0].columns;
}

static void ssd1306_draw_character(
    uint8_t x,
    uint8_t y,
    char character)
{
    const uint8_t *columns = ssd1306_find_glyph(character);
    uint8_t column;
    uint8_t row;

    for (column = 0U; column < SSD1306_GLYPH_WIDTH; column++)
    {
        for (row = 0U; row < SSD1306_GLYPH_HEIGHT; row++)
        {
            const bool pixel_on =
                (columns[column] & (uint8_t)(1U << row)) != 0U;

            ssd1306_draw_pixel(
                (uint8_t)(x + column),
                (uint8_t)(y + row),
                pixel_on);
        }
    }

    for (row = 0U; row < SSD1306_GLYPH_HEIGHT; row++)
    {
        ssd1306_draw_pixel(
            (uint8_t)(x + SSD1306_GLYPH_WIDTH),
            (uint8_t)(y + row),
            false);
    }
}

bool ssd1306_init(const ssd1306_transport_t *transport)
{
    static const uint8_t initialization_commands[] =
    {
        0xAEU,       /* Display off. */
        0xD5U, 0x80U,/* Display clock divide ratio / oscillator. */
        0xA8U, 0x3FU,/* Multiplex ratio: 64 rows. */
        0xD3U, 0x00U,/* Display offset. */
        0x40U,       /* Display start line 0. */
        0x8DU, 0x14U,/* Enable internal charge pump. */
        0x20U, 0x00U,/* Horizontal addressing mode. */
        0xA1U,       /* Segment remap. */
        0xC8U,       /* COM output scan direction remap. */
        0xDAU, 0x12U,/* Alternative COM pin configuration. */
        0x81U, 0x7FU,/* Contrast. */
        0xD9U, 0xF1U,/* Pre-charge period. */
        0xDBU, 0x40U,/* VCOMH deselect level. */
        0xA4U,       /* Resume RAM content display. */
        0xA6U,       /* Normal, non-inverted display. */
        0x2EU,       /* Deactivate scrolling. */
        0xAFU        /* Display on. */
    };

    if ((transport == NULL) ||
        (transport->write == NULL) ||
        (transport->delay_ms == NULL))
    {
        return false;
    }

    s_transport = *transport;

    /*
     * Four-pin modules do not expose RESET, so allow the module's
     * power-on reset circuit to settle before sending commands.
     */
    s_transport.delay_ms(s_transport.power_on_delay_ms);

    if (!ssd1306_write_commands(
            initialization_commands,
            sizeof(initialization_commands)))
    {
        return false;
    }

    ssd1306_clear();

    return ssd1306_update();
}

void ssd1306_clear(void)
{
    size_t index;

    for (index = 0U;
         index < SSD1306_FRAMEBUFFER_SIZE;
         index++)
    {
        s_framebuffer[index] = 0U;
    }
}

void ssd1306_draw_pixel(
    uint8_t x,
    uint8_t y,
    bool illuminated)
{
    size_t framebuffer_index;
    uint8_t pixel_mask;

    if ((x >= SSD1306_WIDTH) ||
        (y >= SSD1306_HEIGHT))
    {
        return;
    }

    framebuffer_index =
        (size_t)x +
        ((size_t)(y / 8U) * SSD1306_WIDTH);
    pixel_mask = (uint8_t)(1U << (y % 8U));

    if (illuminated)
    {
        s_framebuffer[framebuffer_index] |= pixel_mask;
    }
    else
    {
        s_framebuffer[framebuffer_index] &=
            (uint8_t)~pixel_mask;
    }
}

void ssd1306_draw_text(
    uint8_t x,
    uint8_t y,
    const char *text)
{
    uint8_t cursor_x = x;

    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        if ((cursor_x > (SSD1306_WIDTH -
                         SSD1306_GLYPH_ADVANCE)) ||
            (y > (SSD1306_HEIGHT -
                  SSD1306_GLYPH_HEIGHT)))
        {
            break;
        }

        ssd1306_draw_character(cursor_x, y, *text);
        cursor_x =
            (uint8_t)(cursor_x + SSD1306_GLYPH_ADVANCE);
        text++;
    }
}

void ssd1306_draw_progress_bar(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    uint8_t percent)
{
    uint8_t draw_x;
    uint8_t draw_y;
    uint16_t fill_width;

    if ((width < 3U) ||
        (height < 3U) ||
        (x >= SSD1306_WIDTH) ||
        (y >= SSD1306_HEIGHT))
    {
        return;
    }

    if (percent > 100U)
    {
        percent = 100U;
    }

    for (draw_x = 0U; draw_x < width; draw_x++)
    {
        ssd1306_draw_pixel(
            (uint8_t)(x + draw_x),
            y,
            true);
        ssd1306_draw_pixel(
            (uint8_t)(x + draw_x),
            (uint8_t)(y + height - 1U),
            true);
    }

    for (draw_y = 0U; draw_y < height; draw_y++)
    {
        ssd1306_draw_pixel(
            x,
            (uint8_t)(y + draw_y),
            true);
        ssd1306_draw_pixel(
            (uint8_t)(x + width - 1U),
            (uint8_t)(y + draw_y),
            true);
    }

    fill_width =
        ((uint16_t)(width - 2U) * percent + 50U) /
        100U;

    for (draw_y = 1U; draw_y < (height - 1U); draw_y++)
    {
        for (draw_x = 1U; draw_x < (width - 1U); draw_x++)
        {
            ssd1306_draw_pixel(
                (uint8_t)(x + draw_x),
                (uint8_t)(y + draw_y),
                draw_x <= fill_width);
        }
    }
}

bool ssd1306_update(void)
{
    static const uint8_t addressing_commands[] =
    {
        0x21U, 0x00U, 0x7FU, /* Columns 0..127. */
        0x22U, 0x00U, 0x07U  /* Pages 0..7. */
    };

    if (!ssd1306_write_commands(
            addressing_commands,
            sizeof(addressing_commands)))
    {
        return false;
    }

    return ssd1306_write_data(
        s_framebuffer,
        sizeof(s_framebuffer));
}
