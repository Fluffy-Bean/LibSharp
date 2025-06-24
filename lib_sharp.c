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
        display->framebuffer[row + col] |= ~mask;
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

void sharp_display_draw_filled_rectangle(sharp_display_t * display, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    w = min_uint16(w, display->width);
    h = min_uint16(h, display->height);

    for (uint16_t i = 0; i < h; i += 1)
    {
        for (uint16_t j = 0; j < w; j += 1)
        {
            sharp_display_draw_pixel(display, x + j, y + i, color);
        }
    }
}
