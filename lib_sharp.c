#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "sharp_display.h"

uint8_t mirror_bytes(uint8_t byte)
{
    uint8_t res = 0;

    for (uint8_t x = 0; x < 8; x += 1) {
        res = (res << 1) | (byte & 1);
        byte >>= 1;
    }

    return res;
}

void swap_uint16(uint16_t *a, uint16_t *b)
{
    uint16_t t = *a;
    *a = *b;
    *b = t;
}

uint16_t min_uint16(uint16_t a, uint16_t b)
{
    return a < b ? a : b;
}

void swap_int16(int16_t *a, int16_t *b)
{
    int16_t t = *a;
    *a = *b;
    *b = t;
}

sharp_display_t sharp_display_new(uint16_t width, uint16_t height, spi_inst_t * spi, uint8_t cs)
{
    gpio_put(cs, false);

    sharp_display_t display = {
        .width = width,
        .height = height,
        .cs = cs,
        .vcom = CMD_VCOM,
        .framebuffer = (uint8_t *)malloc((width * height) / 8),
        .spi = (spi)
    };

    return display;
}

bool sharp_display_error(sharp_display_t * display)
{
    return display == NULL || display->framebuffer == NULL;
}

void sharp_display_refresh_screen(sharp_display_t * display)
{
    uint8_t spi_byte;

    gpio_put(display->cs, true);
    sharp_display_toggle_vcom(display);

    uint16_t line_size = display->width / 8;

    spi_byte = display->vcom | CMD_WRITE;
    spi_write_blocking(display->spi, &spi_byte, 1);

    for (size_t i = 0; i < display->height; i += 1)
    {
        // Current line
        spi_byte = mirror_bytes(i + 1);
        spi_write_blocking(display->spi, &spi_byte, 1);

        // Line data
        for (size_t j = 0; j < line_size; j += 1)
        {
            spi_byte = mirror_bytes(display->framebuffer[(line_size * i) + j]);
            spi_write_blocking(display->spi, &spi_byte, 1);
        }

        // End of line
        spi_byte = 0b00000000;
        spi_write_blocking(display->spi, &spi_byte, 1);
    }

    // End of transmission
    spi_byte = 0b00000000;
    spi_write_blocking(display->spi, &spi_byte, 1);

    gpio_put(display->cs, false);
}

void sharp_display_clear_screen(sharp_display_t * display)
{
    gpio_put(display->cs, true);
    sharp_display_toggle_vcom(display);

    memset(display->framebuffer, 0b00000000, (display->width * display->height) / 8);

    uint8_t buff[2];
    buff[0] = display->vcom | CMD_CLEAR;
    buff[1] = 0b00000000;

    spi_write_blocking(display->spi, buff, 2);
    gpio_put(display->cs, false);
}

void sharp_display_toggle_vcom(sharp_display_t * display)
{
    display->vcom ^= CMD_VCOM;
}

void sharp_display_set_buffer(sharp_display_t * display, sharp_color_t color)
{
    switch (color) {
    case WHITE:
        memset(display->framebuffer, 0b11111111, (display->width * display->height) / 8);
        break;
    case LIGHT_GRAY:
    case DARK_GRAY:
        for (size_t x = 0; x < display->width; x += 1)
        {
            for (size_t y = 0; y < display->height; y += 1)
            {
                sharp_display_draw_pixel(display, x, y, color);
            }
        }
        break;
    case BLACK:
        memset(display->framebuffer, 0b00000000, (display->width * display->height) / 8);
        break;
    }
}

void sharp_display_draw_pixel(sharp_display_t * display, uint16_t x, uint16_t y, sharp_color_t color)
{
    if (x < 0 || x >= display->width || y < 0 || y >= display->height)
    {
        return;
    }

    uint8_t byte = x % 8;
    uint16_t row = y * (display->width / 8);
    uint16_t col = (x - byte) / 8;
    uint8_t mask = 0b000000001 << byte;

    switch (color) {
    case WHITE:
        display->framebuffer[row + col] |= mask;
        break;
    case LIGHT_GRAY:
        if (x % 3 || y % 3)
        {
            display->framebuffer[row + col] |= mask;
        }
        else
        {
            display->framebuffer[row + col] &= ~mask;
        }
        break;
    case DARK_GRAY:
        if (x % 2)
        {
            display->framebuffer[row + col] |= mask;
        }
        else
        {
            display->framebuffer[row + col] &= ~mask;
        }
        break;
    case BLACK:
        display->framebuffer[row + col] &= ~mask;
        break;
    }
}

void sharp_display_draw_line(sharp_display_t * display, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    // Vertical
    if (x0 == x1)
    {
        if (y0 > y1)
        {
            swap_int16(&y0, &y1);
        }

        for (; y0 < y1; y0 += 1)
        {
            sharp_display_draw_pixel(display, x0, y0, color);
        }

        return;
    }

    // Horizontal
    if (y0 == y1)
    {
        if (x0 > x1)
        {
            swap_int16(&x0, &x1);
        }

        for (; x0 < x1; x0 += 1)
        {
            sharp_display_draw_pixel(display, x0, y0, color);
        }

        return;
    }

    // Bresenham
    int16_t dx = x1 - x0;
    if (dx < 0)
    {
        dx = -dx;
    }

    int16_t dy = y1 - y0;
    if (dy < 0)
    {
        dy = -dy;
    }

    int16_t steep = dy > dx;
    if (steep) {
        swap_int16(&x0, &y0);
        swap_int16(&x1, &y1);
    }
    if (x0 > x1) {
        swap_int16(&x0, &x1);
        swap_int16(&y0, &y1);
    }

    dx = x1 - x0;
    dy = y1 - y0;

    int16_t ystep = 1;
    if (dy < 0) {
        dy = -dy;
        ystep = -1;
    }

    int16_t err = dx / 2;
    for (; x0 <= x1; x0 += 1) {
        if (steep) {
            sharp_display_draw_pixel(display, y0, x0, color);
        } else {
            sharp_display_draw_pixel(display, x0, y0, color);
        }

        err -= dy;

        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void sharp_display_draw_rectangle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    w = min_uint16(w, display->width);
    h = min_uint16(h, display->height);

    sharp_display_draw_line(display, x, y, x + w, y, color);         // Top
    sharp_display_draw_line(display, x, y + h, x + w, y + h, color); // Bottom
    sharp_display_draw_line(display, x, y, x, y + h, color);         // Left
    sharp_display_draw_line(display, x + w, y, x + w, y + h, color); // Right
}

void sharp_display_draw_filled_rectangle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    w = min_uint16(w, display->width);
    h = min_uint16(h, display->height);

    for (; y < h; y += 1)
    {
        sharp_display_draw_line(display, x, y, x + w, y, color);
    }
}

void sharp_display_draw_circle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddfx = 1;
    int16_t ddfy = -2 * r;
    int16_t x0 = 0;
    int16_t y0 = r;

    sharp_display_draw_pixel(display, x, y + r, color);
    sharp_display_draw_pixel(display, x, y - r, color);
    sharp_display_draw_pixel(display, x + r, y, color);
    sharp_display_draw_pixel(display, x - r, y, color);

    while (x0 < y0)
    {
        if (f >= 0)
        {
            y0 -= 1;
            ddfy += 2;
            f += ddfy;
        }

        x0 += 1;
        ddfx += 2;
        f += ddfx;

        sharp_display_draw_pixel(display, x + x0, y + y0, color);
        sharp_display_draw_pixel(display, x - x0, y + y0, color);
        sharp_display_draw_pixel(display, x + x0, y - y0, color);
        sharp_display_draw_pixel(display, x - x0, y - y0, color);
        sharp_display_draw_pixel(display, x + y0, y + x0, color);
        sharp_display_draw_pixel(display, x - y0, y + x0, color);
        sharp_display_draw_pixel(display, x + y0, y - x0, color);
        sharp_display_draw_pixel(display, x - y0, y - x0, color);
    }
}

void sharp_display_draw_filled_circle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddfx = 1;
    int16_t ddfy = -2 * r;
    int16_t x0 = 0;
    int16_t y0 = r;

    sharp_display_draw_line(display, x, y - r, x, y + r, color);

    while (x0 < y0)
    {
        if (f >= 0)
        {
            y0 -= 1;
            ddfy += 2;
            f += ddfy;
        }

        x0 += 1;
        ddfx += 2;
        f += ddfx;

        sharp_display_draw_line(display, x + x0, y - y0, x + x0, y + y0, color);
        sharp_display_draw_line(display, x + y0, y - x0, x + y0, y + x0, color);
        sharp_display_draw_line(display, x - x0, y - y0, x - x0, y + y0, color);
        sharp_display_draw_line(display, x - y0, y - x0, x - y0, y + x0, color);
    }
}

void sharp_display_draw_triangle(sharp_display_t * display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    sharp_display_draw_line(display, x0, y0, x1, y1, color);
    sharp_display_draw_line(display, x1, y1, x2, y2, color);
    sharp_display_draw_line(display, x2, y2, x0, y0, color);
}

void sharp_display_draw_filled_triangle(sharp_display_t * display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    if (y0 > y1) {
        swap_int16(&y0, &y1);
        swap_int16(&x0, &x1);
    }
    if (y1 > y2) {
        swap_int16(&y2, &y1);
        swap_int16(&x2, &x1);
    }
    if (y0 > y1) {
        swap_int16(&y0, &y1);
        swap_int16(&x0, &x1);
    }

    // Line
    if (y0 == y2)
    {
        int16_t a = x0;
        int16_t b = x0;

        if (x1 < a)
        {
            a = x1;
        } else if (x1 > b)
        {
            b = x1;
        }

        if (x2 < a)
        {
            a = x2;
        } else if (x2 > b)
        {
            b = x2;
        }

        sharp_display_draw_line(display, a, y0, b, y0, color);

        return;
    }

    int16_t dx01 = x1 - x0;
    int16_t dy01 = y1 - y0;
    int16_t dx02 = x2 - x0;
    int16_t dy02 = y2 - y0;
    int16_t dx12 = x2 - x1;
    int16_t dy12 = y2 - y1;

    int16_t sa = 0;
    int16_t sb = 0;
    int16_t a = 0;
    int16_t b = 0;

    int16_t last = y1 - 1;
    if (y1 == y2)
    {
        last = y1;
    }

    int16_t y = y0;
    for (; y <= last; y += 1) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;

        sharp_display_draw_line(display, a, y, b, y, color);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);

    for (; y <= y2; y += 1) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;

        sharp_display_draw_line(display, a, y, b, y, color);
    }
}
