#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "semphr.h"

static SemaphoreHandle_t mutex;

/*
Mutexes
    - Mutex = Mutual Exclusion
    - Binary semaphore that includes a priority
    inheritance mechanism
    - A mutex acts as a token that is used to guard
    a resource
        - Like a lock and key. One lock and one key
        that tasks can use one at a time. when the key 
        is used Mutex = 0 and the resource can be accessed
        only by the task that has the key.
            - When Mutex = 1, the resource is locked again and 
            the key can be taken by any task
    - Mutexes protect against two or more taks modifying
    data simultaneously and is a token used to guard data
    - Mutexes shouldn't be used within an interrupt because:
        - They include a priority inheritance mechanism which makes sense
        if the mutex is given and taken from a task, not an interrupt
            - If a higher priority task blocks or waits to obtain a mutex
            held by a lower priority task (called priority inversion), the lower
            priority taks is temporarliy raised to the priority of the blocked task
                - This ensures the higher priority task is blocked the shortest time
                possible
                - However an interrupt is the sole task that needs to be performed
                when triggered, so having the lower priority task finish using the mutex
                so the interrupt can run misses the purpose of an interrupt
        - An interrupt cannot block to wait for a resource that is guarded by
        a mutex to become avaliable
- To enable mutexes in FreeRTOS, configUSE_TIME_SLICING and configUSE_MUTEXES in the 
FreeRTOSConfig.h file must be set to 1
*/


/*EX: Stop Tasks from interrupting each other while printing characters to USB serial*/


void task1(void *pvParameters) // The task function must return void and take a single void * parameter.
{   
    char ch = '1';
    while(1){
    /*
    With no mutexes each task is firing whenever it can, tripping
    over each other creating a mess of characters

        for(int i = 1; i<10; i++){
                putchar(ch);
            }
        puts("");

    With the mutex, the tasks don't interrupt each other, running in blocks
    */
        if(xSemaphoreTake(mutex, 0) == pdTRUE){
            // Semaphore is avaliable
            /*
            Rather than the mutex protecting access to data,
            the mutex controls whether a task can output through 
            USB serial.

            When a task is finished printing 9 times, it relinquishes
            control of the mutex allowing the other task to print. This
            means that even when the task switches it cannot print because
            the mutex has not been given by the other task.

            In other cases, this means global variables can be used by a task
            in its entirety without having to worry about another task altering it
            while the initial task needs it.
            */
            for(int i = 1; i<10; i++){
                putchar(ch);
            }
            puts("");
            xSemaphoreGive(mutex);
        }
        
    }
   
}

void task2(void *pvParameters) // The task function must return void and take a single void * parameter.
{   
    char ch = '2';
    while(1){
        if(xSemaphoreTake(mutex, 0) == pdTRUE){
            // Semaphore is avaliable
            for(int i = 1; i<10; i++){
                putchar(ch);
            }
            puts("");
            xSemaphoreGive(mutex);
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

    mutex = xSemaphoreCreateMutex();

    xTaskCreate(task1, "Task 1", 256, NULL, 1, NULL);
    xTaskCreate(task2, "Task 2", 256, NULL, 1, NULL);    
    vTaskStartScheduler();

    while(1){};
}