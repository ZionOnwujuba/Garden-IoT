#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <queue.h>

/*
RTOS: Real Time Operating System
    - Solves timing problems where one task must wait for 
    another to finish by running tasks individually in a 
    a fashion that seems concurrent, allowing timing constraints
        - The scheduler rather allocates cpu resources for each
        task
        - Disadvantage is that a significant bit of memory must
        be allocated to suspend a task and remembering its state
Task is a set of instructions loaded into memory
    - Return void and accept void param
    - Runs forever with infinite loop
    - No return statement, if task not required must be deleted
A thread is a unit of cpu utilization with its own program counter
and memory 
    - FreeRTOS refers to tasks as something closer to threads
Queue for Task communication
    - Queue: A fifo buffer where if a task writes to a queue it goes
    to the first position. Another write goes to the second position.
    If another task wishes to read data, it reads the first position and
    data in the second position comes to the first position
        - This method of inter-task communication is called thread safe
            - Data between reading and writing can't be interrupted by another
            task
                - Global vars are not thread safe
*/

/*
Task 1: Blink Led, send state of LED to queue
Task 2: Read LED state, print state to console iver usb serial
*/

static QueueHandle_t xQueue = NULL;

void led_task(void *pvParameters) // The task function must return void and take a single void * parameter.
{   
    uint uIValueToSend = 0;
    while (1){
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        uIValueToSend = 1;
        xQueueSend(xQueue, &uIValueToSend, 0U); // Queue name, ptr var to send, delay time if queue is empty
        vTaskDelay(pdMS_TO_TICKS(250));
        

        // Turn the Pico W LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        uIValueToSend = 0;
        xQueueSend(xQueue, &uIValueToSend, 0U);
        vTaskDelay(pdMS_TO_TICKS(250));
        
    }
}

void usb_task(void *pvParameters){
    uint uIRecivedValue;

    while(1){
        xQueueReceive(xQueue, // The handle of the queue where the item is being posted
            &uIRecivedValue, // A pointer to the data you want to place on the queue. 
            portMAX_DELAY); /* The maximum amount of time (in tick periods) the task 
                                should block and wait if the queue is currently full. 
                                If you pass 0, the function returns immediately if the 
                                queue is full. If you pass portMAX_DELAY, the task 
                                will wait indefinitely for space to become available 
                                (provided INCLUDE_vTaskSuspend is set to 1)
                            */

        if(uIRecivedValue == 1){
            printf("LED is ON! \n");
        }
        if(uIRecivedValue == 0){
            printf("LED is OFF! \n");
        }
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

    xQueue = xQueueCreate(1, sizeof(uint)); // Create queue of length one with the size of uint
    xTaskCreate(led_task, "LED_Task", 256, NULL, 1, NULL);
    xTaskCreate(usb_task, "USB_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){};
}