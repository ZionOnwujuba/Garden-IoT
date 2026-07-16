#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "timers.h"

static TaskHandle_t xLEDTaskHandle = NULL;
#define BUTTON_PIN 21
int led_state = 0;

#define DEBOUNCE_DELAY_MS pdMS_TO_TICKS(50) // 50ms cooldown

static TimerHandle_t xDebounceTimer = NULL;

// Debaounce timer func
void vDebounceTimerCallback(TimerHandle_t xTimer) {
    // Re - enable interrupt after delay
    gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
}

void Button_ISR(uint gpio, uint32_t events){
    if(gpio == BUTTON_PIN){
        /*
        A flag managed by FreeRTOS. If the waiting task 
            has a higher priority than the task currently
             running before the interrupt happened, 
             FreeRTOS sets this flag to pdTRUE.
        */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);

        // Notify LED task of button press
        vTaskNotifyGiveFromISR(xLEDTaskHandle, &xHigherPriorityTaskWoken);

        xTimerStartFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
        
        // Context switch if LED task is higher priority
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

}

void led_task(void *pvParameters) // The task function must return void and take a single void * parameter.
{   
    // Save LED task handle to golbal var so ISR knows which task to refer to
    xLEDTaskHandle = xTaskGetCurrentTaskHandle();
    
    while (1){
        // Wait indefinitely until the ISR signals a button press
        // pdTRUE clears the notification count back to 0 on exit
        // portMAX_DELAY tells it to sleep forever until the button 
            // notification arrives. 
        // Once notified, it wakes up, toggles the LED pin, and loops 
            // back around to sleep again.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Blocks task until notified

        led_state = !led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
    }
}

void idle_task(void *pvParameters){
    while(1){
        printf("Idle Task \n");
    }
}


int main()
{
    stdio_init_all();

    gpio_init(BUTTON_PIN); // Push button connected to pin 20
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // CRITICAL: Initialize the wireless chip framework before using the LED
    if (cyw43_arch_init()) {
        printf("Wi-Fi architecture initialization failed!\n");
        return -1;  
    }

    // Software timer
     xDebounceTimer = xTimerCreate(
        "DebounceTimer",        // Text name
        DEBOUNCE_DELAY_MS,      // Timer period (50ms)
        pdFALSE,                // pdFALSE = Timer runs then stops
        (void *)0,              // Timer ID
        vDebounceTimerCallback  // Callback function
    );

    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &Button_ISR);
    xTaskCreate(led_task, 
        "LED_Task", 
        256, // Stack size (words)
        NULL, 
        2, 
        NULL);
    xTaskCreate(led_task, "LED_Task", 256,  NULL, 1, NULL);
    vTaskStartScheduler();

    while(1){};
}