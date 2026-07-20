#ifndef LCD_H
#define LCD_H

void lcd_send(uint8_t mode, uint8_t value);

void lcd_command(uint8_t value);

void lcd_write_char(uint8_t value);

void gravity_lcd_init(void);

void gravity_set_rgb(uint8_t r, uint8_t g, uint8_t b);

void lcd_set_cursor(uint8_t col, uint8_t row);

void lcd_print(const char *str);

void lcd_init(void);

#endif