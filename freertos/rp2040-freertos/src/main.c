#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "rs485.h"
#include "dht22.h"
#include "lcd.h"
#include "light_sensor.h"
#include "constants.h"

typedef struct {
    float soil_tempF;
    float soil_tempC;
    float soil_hum;
    float soil_ph;
    float ambient_tempF;
    float ambient_tempC;
    float ambient_hum;
    float lux;
} sens_package;

static QueueHandle_t xQueue = NULL;

void led_task(void *pvParameters) 
{   
    while (1){
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));

        // Turn the Pico W LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
        
    }
}

void data_aquisition_task(void *pvParameters){
    dht_data current_readings;
    rs485_data rsdata;
    sens_package sensorPackage;
    
    while (1) {
        
        if (read_from_dht(&current_readings)) {
            float fahrenheit = (current_readings.temperature * 9 / 5) + 32;
            sensorPackage.ambient_tempF = fahrenheit;
            sensorPackage.ambient_hum = current_readings.humidity;
            sensorPackage.ambient_tempC = current_readings.temperature;
        } else {
            printf("Failed to read data from DHT22 (Checksum/Timeout Error).\n");
        }
        readHumiturePH(&rsdata);
        float fahrenheit_rs = (rsdata.tem * 9 / 5) + 32;
        sensorPackage.soil_tempC = rsdata.tem;
        sensorPackage.soil_tempF = fahrenheit_rs;
        sensorPackage.soil_hum = rsdata.hem;
        sensorPackage.soil_ph = rsdata.ph;
        sensorPackage.lux = bh1750_read_light(I2C_PORT_BF1750) / 1.2;
        xQueueSend(xQueue, &sensorPackage, 0U);
        vTaskDelay(2000);
    }
    
}




int main()
{
    stdio_init_all();

     // Initialize chosen DHT22 pin
    gpio_init(DHT_PIN);

    // Initiialize lcd
    lcd_init();

    bh1750_init(I2C_PORT_BF1750);

    rs485_init();

    // CRITICAL: Initialize the wireless chip framework before using the LED
    if (cyw43_arch_init()) {
        printf("Wi-Fi architecture initialization failed!\n");
        return -1;
    }

    xTaskCreate(led_task, "LED_Task", 256, NULL, 1, NULL);
    xTaskCreate(
            led_task,       // Function pointer
            "LED_Task",    // Task name for debugging
            256,           // Stack depth
            NULL, // Parameter to pass (cast to void*)
            1,             // Priority
            NULL       // Task handle
    );
    vTaskStartScheduler();

    while(1){};
}