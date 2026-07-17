/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif

const uint DHT_PIN = 15;
const uint MAX_TIMINGS = 85;


typedef struct {
    float humidity;
    float temperature;
} dht_data;


  

bool read_from_dht(dht_data *result) {
    int data[5] = {0, 0, 0, 0, 0};
    uint last_state = 1;
    uint j = 0;
    /*
    The DHT22 stays in a low-power sleep mode until the 
    Pico wakes it up. The Pico initiates communication 
    by pulling the data line down and then letting it float back up.
    */
    // 1. Send out the start signal sequence
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(18); // Pull low for at least 18ms
    
    gpio_put(DHT_PIN, 1);
    sleep_us(40); // Pull high for 20-40us

    // 2. Prepare to read response pulses from the sensor
    gpio_set_dir(DHT_PIN, GPIO_IN);

    // Loop to read high/low transitions from the DHT22
        /*
The sensor transmits 40 bits (5 bytes total) of data. 
Every single bit begins with a 50 μs low pulse. 
The length of the subsequent high pulse determines 
whether the bit is a 0 or a 1:
    Bit '0': High pulse lasts 26–28 μs.
    Bit '1': High pulse lasts 70 μs
*/
    for (uint i = 0; i < MAX_TIMINGS; i++) {
        uint counter = 0;
        while (gpio_get(DHT_PIN) == last_state) {
            counter++;
            sleep_us(1);
            if (counter == 255) break;
        }
        last_state = gpio_get(DHT_PIN);

        if (counter == 255) break;

        // Skip the first 3 transitional responses, parse only data transitions
        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            // If the pulse duration was long, it represents a '1' bit
            if (counter > 30) { 
                data[j / 8] |= 1;
            }
            j++;
        }
    }
    /*
The 5 bytes (data[0] to data[4]) hold the raw information:
    Bytes 0 & 1: Relative Humidity (multiplied by 10).
    Bytes 2 & 3: Temperature (multiplied by 10).
    Byte 4: Checksum.
        Checksum Validation: 
            The code adds the first 4 bytes together. 
            The lowest 8 bits of this sum must match the
             5th byte (checksum). If they do not match, 
             the data is corrupted and discarded.
*/

    // 3. Verify checksum and parse raw data array
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        // DHT22 scales values by multiplying them by 10

        /*
        To get the actual float values, the code combines the bytes 
        and divides by 10. For temperature, it also checks the highest 
        bit (0x8000) because the DHT22 flags negative temperatures 
        by turning this bit on.
        */
        float h = (float)((data[0] << 8) + data[1]) / 10.0;
        
        // Temperature parsing with negative bit checking (MSB)
        float t = (float)((data[2] & 0x7F) << 8 | data[3]) / 10.0;
        if (data[2] & 0x80) {
            t = -t;
        }
        
        result->humidity = h;
        result->temperature = t;
        return true;
    }
    
    return false; // Checksum error or bad transmission timeout
}

int main() {
    stdio_init_all();
    printf("DHT22 Sensor Initialization\n");

    // Initialize chosen GPIO pin
    gpio_init(DHT_PIN);

    while (1) {
        dht_data current_readings;
        
        if (read_from_dht(&current_readings)) {
            float fahrenheit = (current_readings.temperature * 9 / 5) + 32;
            printf("Humidity: %.1f%%, Temperature: %.1f°C (%.1fF)\n", 
                   current_readings.humidity, current_readings.temperature, fahrenheit);
        } else {
            printf("Failed to read data from DHT22 (Checksum/Timeout Error).\n");
        }

        // The DHT22 updates values slowly; sample interval must be >= 2 seconds
        sleep_ms(2000); 
    }
}