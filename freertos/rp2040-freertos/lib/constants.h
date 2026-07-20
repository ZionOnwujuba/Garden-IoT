#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "hardware/i2c.h"

// DHT22 config
#define DHT_PIN 15
#define MAX_TIMINGS 85

// BH1750 Pin Assignments
#define ADDR 0x23 // Default I2C address for BH1750
#define I2C_PORT_BF1750 i2c1
#define SDA_PIN_BF1750 2
#define SCL_PIN_BF1750 3

// Define I2C LCD Pin Assignments
#define I2C_PORT_LCD i2c0
#define SDA_PIN_LCD 0
#define SCL_PIN_LCD 1

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

// RS485 UART config
#define UART_ID_RS485         uart0
#define BAUDRATE_RS485        9600
#define UART_TX_PIN_RS485     12
#define UART_RX_PIN_RS485     13

#endif 