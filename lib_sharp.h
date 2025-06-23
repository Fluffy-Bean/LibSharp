#ifndef SHARP_DISPLAY
#define SHARP_DISPLAY

#include "hardware/spi.h"

// in LSB format
#define CMD_WRITE 0b10000000
#define CMD_VCOM  0b01000000
#define CMD_CLEAR 0b00100000

typedef enum sharp_color
{
    WHITE = 0,
    BLACK = 1,
}
sharp_color_t;

typedef struct sharp_display
{
    uint16_t width, height;
    uint8_t cs;
    uint8_t vcom;
    uint8_t * framebuffer;
    spi_inst_t * spi;
}
sharp_display_t;

sharp_display_t sharp_display_new(uint16_t width, uint16_t height, spi_inst_t * spi, uint8_t cs);
bool sharp_display_error(sharp_display_t * display);
void sharp_display_refresh_screen(sharp_display_t * display);
void sharp_display_clear_screen(sharp_display_t * display);
void sharp_display_toggle_vcom(sharp_display_t * display);
void sharp_display_draw_pixel(sharp_display_t *display, uint16_t x, uint16_t y, sharp_color_t color);
void sharp_display_fill_screen(sharp_display_t * display, sharp_color_t color);

#endif
