#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// Remote change added
void led_task(void *pvParameters) // The task function must return void and take a single void * parameter.
{   
    while (1){
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));

        // Turn the Pico W LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

int main()
{
    stdio_init_all();

    // CRITICAL: Initialize the wireless chip framework before using the LED
    if (cyw43_arch_init()) {
        printf("Wi-Fi architecture initialization failed!\n");
        return -1;
    }

    xTaskCreate(led_task, "LED_Task", 256, NULL, 1, NULL);
    vTaskStartScheduler();

    while(1){};
}