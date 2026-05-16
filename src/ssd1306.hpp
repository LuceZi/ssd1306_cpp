#ifndef SSD1306_HPP
#define SSD1306_HPP

#pragma once

#include "stdio.h"
#include <Wire.h>

#include "static_resource/fonts.c"
#include "static_resource/icons.c"
#include "static_resource/images.c"

// #include "../shared_data.h" // for received_data_t and dataQueue

#define I2C_BLOCK_MAX 32

static const uint8_t inital_commands[] = {
    0xAE,       // Display off
    0xD5, 0x80, // Set display clock divide ratio/oscillator frequency
    0xA8, 0x3F, // Set multiplex ratio (1 to 64)
    0xD3, 0x00, // Set display offset
    0x40,       // Set start line address
    0x8D, 0x14, // Charge pump setting (enable)
    0x20, 0x00, // Memory addressing mode (horizontal addressing mode)
    0xA1,       // Set segment re-map (column address 127 is mapped to SEG0)
    0xC8,       // Set COM output scan direction (remapped mode)
    0xDA, 0x12, // Set COM pins hardware configuration
    0x81, 0xCF, // Set contrast control
    0xD9, 0xF1, // Set pre-charge period
    0xDB, 0x40, // Set VCOMH deselect level
    0xA4,       // Entire display ON (resume to RAM content display)
    0xA6,       // Set normal display (not inverted)
    0xAF        // Display ON
};

// declaration area
class SSD1306
{
private:
    TaskHandle_t taskHandle = NULL;
    uint8_t oled_addr;
    bool active = false;
    //uint32_t image_carousel_timer = 0;
    uint8_t recent_image_index = 0;
    uint32_t reverse_threshold = 0;
    uint8_t x_shift_range = 0; // for future use

    void oled_set_position(uint8_t page, uint8_t column);
    void print_chr(uint8_t page, uint8_t column, char data, bool is_number = 0);
    void run(); // vTaskDelay(pdMS_TO_TICKS(100));

public:
    // Constructor
    SSD1306(uint8_t address = 0x3C, uint32_t image_carousel_timer = 5000, uint32_t reverse_threshold = 255)
    {
        this->oled_addr = address;
        this->reverse_threshold = reverse_threshold;
        //this->image_carousel_timer = image_carousel_timer;
    }

    //=== runtime methods ===
    void begin();
    void stop();

    //=== public methods ===
    void oled_write_page_data(uint8_t page, uint8_t column, const uint8_t *data, size_t len);
    void ssd1306_init();
    void ssd1306_clear();
    void ssd1306_full_light();
    void ssd1306_reverse();
    void ssd1306_shift(int8_t y_shift = 0);
    void display_string(uint8_t start_page, uint8_t start_column, const char *text);
    void display_icons(uint8_t start_page, uint8_t start_col, const uint8_t icon[4][8]);
    void display_image(uint8_t page, uint8_t col, const uint8_t *data);
};

#endif