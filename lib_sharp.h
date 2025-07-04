#ifndef SHARP_DISPLAY
#define SHARP_DISPLAY

#include "hardware/spi.h"

// LSB format
#define CMD_WRITE 0b10000000
#define CMD_VCOM  0b01000000
#define CMD_CLEAR 0b00100000

typedef enum sharp_color
{
    WHITE,
    LIGHT_GRAY,
    DARK_GRAY,
    BLACK,
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

void sharp_display_set_buffer(sharp_display_t * display, sharp_color_t color);

void sharp_display_draw_pixel(sharp_display_t * display, uint16_t x, uint16_t y, sharp_color_t color);
void sharp_display_draw_line(sharp_display_t * display, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void sharp_display_draw_rectangle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void sharp_display_draw_filled_rectangle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void sharp_display_draw_circle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t r, uint16_t color);
void sharp_display_draw_filled_circle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t r, uint16_t color);
void sharp_display_draw_triangle(sharp_display_t * display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void sharp_display_draw_filled_triangle(sharp_display_t * display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

#endif
