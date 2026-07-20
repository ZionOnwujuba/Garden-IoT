#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "light_sensor.h"
#include "constants.h"


const uint8_t POWER_ON = 0x01;
const uint8_t CONTINUOUS_HIGH_RES_MODE = 0x10;

void bh1750_init(i2c_inst_t *i2c) {
    // Initialize I2C at 400kHz
    i2c_init(I2C_PORT_BF1750, 400 * 1000);
    gpio_set_function(SDA_PIN_BF1750, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_BF1750, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_BF1750);
    gpio_pull_up(SCL_PIN_BF1750);
    uint8_t cmd = POWER_ON;
    i2c_write_blocking(i2c, ADDR, &cmd, 1, false);
    cmd = CONTINUOUS_HIGH_RES_MODE;
    i2c_write_blocking(i2c, ADDR, &cmd, 1, false);
}

uint16_t bh1750_read_light(i2c_inst_t *i2c) {
    uint8_t buffer[2] = {0, 0};
    i2c_read_blocking(i2c, ADDR, buffer, 2, false);
    uint16_t val = (buffer[0] << 8 | buffer[1]);
    return val;
}
/*
int main() {
    stdio_init_all();
    
    // Initialize I2C at 400kHz
    i2c_init(I2C_PORT_BF1750, 400 * 1000);
    gpio_set_function(SDA_PIN_BF1750, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_BF1750, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_BF1750);
    gpio_pull_up(SCL_PIN_BF1750);

    bh1750_init(I2C_PORT_BF1750);

    while (true) {
        uint16_t raw_light = bh1750_read_light(I2C_PORT_BF1750);
        float lux = raw_light / 1.2; // Convert raw reading to Lux
        printf("Luminosity: %.2f lx\n", lux);
        sleep_ms(1000);
    }
    return 0;
}
*/