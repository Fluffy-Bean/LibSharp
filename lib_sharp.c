#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "lib_sharp.h"

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
    return (display == NULL || display->framebuffer == NULL);
}

void sharp_display_refresh_screen(sharp_display_t * display)
{
    gpio_put(display->cs, true);
    sharp_display_toggle_vcom(display);

    // These are sizes without the current line and end of line data
    uint16_t line_size = display->width / 8;
    uint16_t frame_size = (display->width * display->height) / 8;

    uint16_t buff_size = 1 + display->height * (1 + (display->width / 8) + 1) + 1;
    uint8_t buff[buff_size];

    uint16_t buff_offset = 0;

    buff[buff_offset] = display->vcom | CMD_WRITE;
    buff_offset += 1;

    for (int i = 0; i < frame_size; i += line_size)
    {
        // Current line
        buff[buff_offset] = ((i + 1) / (display->width / 8)) + 1;
        buff_offset += 1;

        // Line data
        memcpy(buff + buff_offset, display->framebuffer + i, line_size);
        buff_offset += line_size;

        // End of line
        buff[buff_offset] = 0x00;
        buff_offset += 1;
    }

    // End of transition
    buff[buff_offset] = 0x00;
    buff_offset += 1;

    if (buff_size != buff_offset)
    {
        printf("Sussy buffer size offset\n");
    }

    spi_write_blocking(display->spi, buff, buff_offset);
    gpio_put(display->cs, false);
}

void sharp_display_clear_screen(sharp_display_t * display)
{
    sharp_display_clear_buffer(display);
    gpio_put(display->cs, true);

    uint8_t buff[2] = {display->vcom | CMD_CLEAR, 0x00};

    spi_write_blocking(display->spi, buff, 2);

    sharp_display_toggle_vcom(display);
    gpio_put(display->cs, false);
}

void sharp_display_clear_buffer(sharp_display_t * display)
{
    memset(display->framebuffer, 0, display->width * display->height / 8);
}

void sharp_display_toggle_vcom(sharp_display_t * display)
{
    display->vcom = display->vcom ? 0x00 : CMD_VCOM;
}
