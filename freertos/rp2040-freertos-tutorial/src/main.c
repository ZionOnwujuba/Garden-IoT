#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "semphr.h"


SemaphoreHandle_t count;

/*
Semaphores
- Binary Semaphore:
    - Similar to mutexes but without priority inheritance
        - Priority inheritence ensures tasks aren't left waiting
        for a lower priority task to finish using a mutex, by raising
        the priority of the lower priority task and allowing it to finish
        its use of the shared resource
    - 'Queue' that can hold only one item which has a value
    of 0 or 1
    - Binary semaphores are the better choice for implementing
    task synchronisation
    - Mutexes are the better choice for implementing mutual
    exclusion or resource protection
    - EX: If there is a task to service the peripheral, rather than constantly
    polling the peripheral, an interupt routing gives a semaphore when there
    is something to do with the peripheral. The task can then take the semaphore
    and service the peripheral (the task does not give the semaphore back in this
    scenario)
- Counting Semaphore
    - 'Queue' that can have multiple items
    - The data in the queue is not important, only whether the queue is
    full or empty
    - Counting semaphores are used for counting events and resource management
    - EX: If there as event handler that gives a semaphore every time something occurs,
    incrementing the value of the semaphore. A handler task can then take the semaphore
    and process the event, decrementing the semaphore. The value of the semaphore is:
        - # of events that have occured - # of events that have been processed
        - This method keeps track of events and ensuring the a processed properly
        even if there is a backlog
    - EX: If the count of a semaphore indicated the number of resorues avaliable. To control the
    resource, the task must take a semaphore, ecrementing the semaphore value, indicating 
    there are less resources to use. If there are no semaphores remaining, there are no resources
    for a task to use. Once the task has finished using the resource, it gives the semaohore back

*/

/*
EX: Implement a counting semaphore, event counter
Push a button -> LED on for a second
*/

void led_task(void *pvParameters) // The task function must return void and take a single void * parameter.
{   
    while (1){
        // Task polls to see if a semaphore can be taken
        if(xSemaphoreTake(count, (TickType_t) 10) == pdTRUE){ // Args: Semaphore we are taking, time in ticks 
                                                              // to wait for semaphore to become avaliable 
            // Can take semaphore, turn on led
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));                                        

        } else {
            // Can't take semaphore, Turn the Pico W LED off
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1));  
        }
    }
}

void button_task(void *pvParameters){
    gpio_init(20); // Push button connected to pin 20
    gpio_set_dir(20, GPIO_IN);
    // Task polls to see if button is pressed
    while(1){
        // If button is pressed, increment semaphore
        if(gpio_get(20) != 0){
            xSemaphoreGive(count);
            vTaskDelay(pdMS_TO_TICKS(20));
        } 
        else {
            // If button is not pressed, just delay
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

}

int main()
{
    stdio_init_all();

    count = xSemaphoreCreateCounting(5, 0); // Max count of 5, initial count of 0

    // CRITICAL: Initialize the wireless chip framework before using the LED
    if (cyw43_arch_init()) {
        printf("Wi-Fi architecture initialization failed!\n");
        return -1;  
    }

    xTaskCreate(led_task, "LED_Task", 256, NULL, 1, NULL);
    xTaskCreate(button_task, "Button_Task", 256, NULL, 1, NULL);
    vTaskStartScheduler();

    while(1){};
}