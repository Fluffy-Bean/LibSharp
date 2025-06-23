#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "sharp_display.h"

uint8_t mirror_bytes(uint8_t byte)
{
    uint8_t res = 0;

    for (uint8_t x=0; x < 8; ++x) {
        res = (res << 1) | (byte & 1);
        byte >>= 1;
    }

    return res;
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

    gpio_put(display->cs, 1);
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

    gpio_put(display->cs, 0);
}

void sharp_display_clear_screen(sharp_display_t * display)
{
    gpio_put(display->cs, 1);
    sharp_display_toggle_vcom(display);

    memset(display->framebuffer, 0b00000000, (display->width * display->height) / 8);

    uint8_t buff[2];
    buff[0] = display->vcom | CMD_CLEAR;
    buff[1] = 0b00000000;

    spi_write_blocking(display->spi, buff, 2);
    gpio_put(display->cs, 0);
}

void sharp_display_toggle_vcom(sharp_display_t * display)
{
    display->vcom ^= CMD_VCOM;
}

void sharp_display_draw_pixel(sharp_display_t *display, uint16_t x, uint16_t y, sharp_color_t color)
{
    if (x < 0 || x >= display->width || y < 0 || y >= display->height) return;

    uint16_t pixel_in_byte = x % 8;
    uint16_t column_in_bytes = (x - pixel_in_byte) / 8;
    uint8_t mask = 0b000000001 << pixel_in_byte;

    if (color == WHITE) {
        display->framebuffer[y*(display->width / 8) + column_in_bytes] |= ~mask;
    } else {
        display->framebuffer[y*(display->width / 8) + column_in_bytes] &= ~mask;
    }
}

void sharp_display_fill_screen(sharp_display_t * display, sharp_color_t color)
{
    if (color == WHITE) {
        memset(display->framebuffer, 0b11111111, (display->width * display->height) / 8);
    } else {
        memset(display->framebuffer, 0b00000000, (display->width * display->height) / 8);
    }
}
