#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_SERVICE_WIDTH  (128U)
#define DISPLAY_SERVICE_HEIGHT (64U)

bool display_service_init(void);
void display_service_clear(void);
void display_service_draw_text(uint8_t x, uint8_t y, const char *text);
void display_service_draw_progress_bar(uint8_t x,
                                       uint8_t y,
                                       uint8_t width,
                                       uint8_t height,
                                       uint8_t percent);
bool display_service_present(void);

#endif
