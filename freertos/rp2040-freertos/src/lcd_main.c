/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Define I2C Pin Assignments
#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

// DFRobot Gravity LCD1602 I2C Target Addresses
#define LCD_ADDRESS 0x3E   // Fixed LCD character controller address
#define RGB_ADDRESS 0x60   // RGB backlight controller address (Common for V1.0)

// LCD Control Registers / Mode bytes
#define COMMAND_MODE 0x80
#define DATA_MODE    0x40

// Core Display Commands
#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET    0x20

// Display On/Off Flags
#define LCD_DISPLAYON  0x04
#define LCD_CURSOROFF  0x00
#define LCD_BLINKOFF   0x00
#define LCD_2LINE      0x08

// Backlight State Masks
#define LCD_BACKLIGHTON    0x08  // Bit mask to turn backlight ON
#define LCD_BACKLIGHTOFF   0x00  // Bit mask to turn backlight OFF


// Helper to transmit data chunks over I2C safely
void lcd_send(uint8_t mode, uint8_t value) {
    uint8_t buffer[2] = {mode, value};
    i2c_write_blocking(I2C_PORT, LCD_ADDRESS, buffer, 2, false);
}

// Write individual command bytes
void lcd_command(uint8_t value) {
    lcd_send(COMMAND_MODE, value);
}

// Write individual character bytes
void lcd_write_char(uint8_t value) {
    lcd_send(DATA_MODE, value);
}

// Initialize the screen properties
void gravity_lcd_init() {
    // Wait for the Gravity internal MCU to power up cleanly
    sleep_ms(50); 
    
    // Configure display layout: 2 lines, default 5x8 font size
    lcd_command(LCD_FUNCTIONSET | LCD_2LINE);
    sleep_ms(5);
    
    // Turn display on, deactivate cursors and blinking behaviors
    lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF | LCD_BACKLIGHTON);
    
    // Flush active RAM and clear text
    lcd_command(LCD_CLEARDISPLAY);
    sleep_ms(5);
}

// Drive RGB Backlight configuration 
void gravity_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    // Map of standard DFRobot RGB control registers
    uint8_t reg_mode1[2]  = {0x00, 0x00};
    uint8_t reg_output[2] = {0x08, 0xAA}; // Enable PWM modes on outputs
    uint8_t reg_blue[2]   = {0x02, b};
    uint8_t reg_green[2]  = {0x03, g};
    uint8_t reg_red[2]    = {0x04, r};

    i2c_write_blocking(I2C_PORT, RGB_ADDRESS, reg_mode1, 2, false);
    i2c_write_blocking(I2C_PORT, RGB_ADDRESS, reg_output, 2, false);
    i2c_write_blocking(I2C_PORT, RGB_ADDRESS, reg_red, 2, false);
    i2c_write_blocking(I2C_PORT, RGB_ADDRESS, reg_green, 2, false);
    i2c_write_blocking(I2C_PORT, RGB_ADDRESS, reg_blue, 2, false);
}

// Position screen cursor (Line index 0 or 1)
void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t val = (row == 0) ? (col | 0x80) : (col | 0xc0);
    lcd_command(val);
}

// Print a standard C string to screen
void lcd_print(const char *str) {
    while (*str) {
        lcd_write_char(*str++);
    }
}

int main() {
    stdio_init_all();
    
    // Set up standard Pico I2C block at 100kHz standard speed
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    
    // Boot sequence
    gravity_lcd_init();
    
    // Set backlight color (Optional: Red = 255, Green = 0, Blue = 128)
    gravity_set_rgb(255, 0, 128);
    
    // Output Text
    lcd_set_cursor(0, 0);
    lcd_print("Gravity LCD1602");
    
    lcd_set_cursor(0, 1);
    lcd_print("Pico W C-SDK!");
    
    while (1) {
        tight_loop_contents();
    }
}
