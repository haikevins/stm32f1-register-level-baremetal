#include "application.h"

#include <stddef.h>

#include "application_config.h"
#include "display_service.h"
#include "time_service.h"

static uint32_t g_last_update_ms;
static bool g_progress_increasing;
static bool g_display_operational;

volatile uint8_t application_display_progress_percent;
volatile uint32_t application_display_update_count;
volatile uint32_t application_display_error_count;

static void format_unsigned(uint32_t value,
                            char *buffer,
                            size_t capacity)
{
    char reversed[10];
    size_t digit_count = 0U;
    size_t output_index;

    if ((buffer == NULL) || (capacity == 0U))
    {
        return;
    }

    do
    {
        reversed[digit_count] =
            (char)('0' + (char)(value % 10U));
        digit_count++;
        value /= 10U;
    }
    while ((value != 0U) &&
           (digit_count < sizeof(reversed)));

    if (digit_count >= capacity)
    {
        digit_count = capacity - 1U;
    }

    for (output_index = 0U;
         output_index < digit_count;
         output_index++)
    {
        buffer[output_index] =
            reversed[digit_count - output_index - 1U];
    }

    buffer[digit_count] = '\0';
}

static void render_frame(void)
{
    char seconds_text[11];
    const uint32_t elapsed_seconds =
        time_service_now_ms() / 1000U;

    format_unsigned(elapsed_seconds,
                    seconds_text,
                    sizeof(seconds_text));

    display_service_clear();
    display_service_draw_text(0U, 0U, "STM32F103");
    display_service_draw_text(0U, 11U, "I2C SSD1306");
    display_service_draw_text(0U, 27U, "UPTIME");
    display_service_draw_text(48U, 27U, seconds_text);
    display_service_draw_text(0U, 38U, "SECONDS");
    display_service_draw_progress_bar(
        0U,
        52U,
        DISPLAY_SERVICE_WIDTH,
        12U,
        application_display_progress_percent);
}

static void advance_progress(void)
{
    if (g_progress_increasing)
    {
        if (application_display_progress_percent >=
            (uint8_t)(100U - DISPLAY_DEMO_PROGRESS_STEP))
        {
            application_display_progress_percent = 100U;
            g_progress_increasing = false;
        }
        else
        {
            application_display_progress_percent =
                (uint8_t)(application_display_progress_percent +
                          DISPLAY_DEMO_PROGRESS_STEP);
        }
    }
    else
    {
        if (application_display_progress_percent <=
            DISPLAY_DEMO_PROGRESS_STEP)
        {
            application_display_progress_percent = 0U;
            g_progress_increasing = true;
        }
        else
        {
            application_display_progress_percent =
                (uint8_t)(application_display_progress_percent -
                          DISPLAY_DEMO_PROGRESS_STEP);
        }
    }
}

bool application_init(void)
{
    g_last_update_ms = time_service_now_ms();
    g_progress_increasing = true;
    g_display_operational = true;

    application_display_progress_percent = 0U;
    application_display_update_count = 0U;
    application_display_error_count = 0U;

    render_frame();

    if (!display_service_present())
    {
        application_display_error_count++;
        g_display_operational = false;
        return false;
    }

    return true;
}

void application_process(void)
{
    if (!g_display_operational)
    {
        return;
    }

    if (!time_service_periodic_due(&g_last_update_ms,
                                   DISPLAY_DEMO_UPDATE_PERIOD_MS))
    {
        return;
    }

    advance_progress();
    render_frame();

    if (!display_service_present())
    {
        application_display_error_count++;
        g_display_operational = false;
        return;
    }

    application_display_update_count++;
}
