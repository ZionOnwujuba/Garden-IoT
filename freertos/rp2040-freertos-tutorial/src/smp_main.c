#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "semphr.h"

/*
Multicore FreeRTOS Architecture
    Asymetric Multiprocessing
        - Each core runs its on instance of FreeRTOS
        - Core can have different architectures, but some 
        share memory rquired
        - Each core runs its own scheduler
    Symmetric Multiprocessing
        - Single instance of FreeRTOS schedules tasks
        across cores
        - Only one port of FreeRTOS can be used so the 
        architecture of each core must the the same
        - SMP vs Single Core
            - In SMP environments, more than one task
            can one at once
            - A lower priority task can run alongside 
            a higher priority taks (including ISRs)
            - Tasks that are in a blocked or suspended 
            states are not able to run
            - Tasks can run at the same time as ISRs
            - If configRUN_MULTIPLE_PRIORITIES is set to 0 
            then only tasks with equal priorities will run
            simultaneously
- "FreeRTOS preemption is a scheduling mechanism that allows 
    the operating system to forcibly pause a currently running 
    task so a higher-priority task can execute immediately, 
    or to switch between tasks of equal priority. 
        By default, FreeRTOS uses a preemptive scheduler, meaning critical, 
        high-priority tasks do not have to wait for lower-priority 
        tasks to finish their work."
FreeRTOS SMP Functions (all void funcs)
    - vTaskCoreAffinitySet(handle, mask)
        - Pin a task to specific core
        - Mask for core 0 would be (1 << 0)
    - vTaskCoreAffinityGet(handle)
        - Returns core affnity mask
    - vTaskPreemptionDisable(handle)
        - Disables task pre-emption
    - vTaskPreemptionEnable(handle)
        - Enables task pre-emption

*/

const int task_delay = 500;
const int task_size = 128;

SemaphoreHandle_t mutex;

void vGuardedPrint (char *out){ // Print serial output function to 
                                // ensure only on core prints at a time
    xSemaphoreTake(mutex, portMAX_DELAY);
    puts(out);
    xSemaphoreGive(mutex);
}

void vTaskSMP_print_core(void *pvParameters){
    char *task_name = pcTaskGetName(NULL);
    char out[12];
    while(1){
        sprintf(out, "%s %d", task_name, get_core_num());
        vGuardedPrint(out);
        vTaskDelay(task_delay);
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
    TaskHandle_t handleA;
    TaskHandle_t handleB;



    xTaskCreate(vTaskSMP_print_core, "Task A", task_size, NULL, 1, &handleA);
    xTaskCreate(vTaskSMP_print_core, "Task B", task_size, NULL, 1, &handleB);
    xTaskCreate(vTaskSMP_print_core, "Task C", task_size, NULL, 1, NULL);
    xTaskCreate(vTaskSMP_print_core, "Task D", task_size, NULL, 1, NULL);

    vTaskCoreAffinitySet(handleA, (1 << 0)); // Core 0
    vTaskCoreAffinitySet(handleB, (1 << 1)); // Core 1

    vTaskStartScheduler();

    while(1){};
}