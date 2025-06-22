#ifndef LIB_SHARP_H
#define LIB_SHARP_H

#include "hardware/spi.h"

#define CMD_WRITE 0x80
#define CMD_CLEAR 0x20
#define CMD_VCOM 0x40

#define SWAP(a, b) { a ^= b; b ^= a; a ^= b; }

typedef struct sharp_display {
    uint16_t width, height;
    uint8_t cs;
    uint8_t vcom;
    uint8_t * framebuffer;
    spi_inst_t * spi;
} sharp_display_t;

sharp_display_t sharp_display_new(uint16_t width, uint16_t height, spi_inst_t * spi, uint8_t cs);
bool sharp_display_error(sharp_display_t * display);
void sharp_display_refresh_screen(sharp_display_t * display);
void sharp_display_clear_screen(sharp_display_t * display);
void sharp_display_clear_buffer(sharp_display_t * display);
void sharp_display_toggle_vcom(sharp_display_t * display);

void sharp_display_draw_pixel(sharp_display_t *display, int x, int y, bool color);
void sharp_display_draw_circle(sharp_display_t *display, int x0, int y0, int radius, bool color);

#endif //LIB_SHARP_H
