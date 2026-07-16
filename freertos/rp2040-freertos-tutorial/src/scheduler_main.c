#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"


/*
Scheduling Algorithm
    - Decides with RTOS task to be running at a given time
    - Can only be one running at the same time per core
- FreeRTOS default scheduler
    - Fixed priority: scheduler will not change task priority
    permanently
    - Pre-emptive: scheduler will always run the highest priority
    RTOS task that is able to run, regardless of when a task becomes
    able to run (even within time slice)
    - Round robin: Tasks that share a priority take turns running. The time
    within a task being able to run (time within t1 and t2) is called a time slice
        - T1 and T2 themselves are tick interrupts
    - Time sliced: tasks that share a priority take turns entering the 
    running state (scheduler switches between tasks with equal priority
    on each tick interrupt)
        - To enable time slicing in FreeRTOS, configUSE_TIME_SLICING in the 
        FreeRTOSConfig.h file must be set to 1
- When a tick intrrupt occurs the kernal runs in tick interrupt to select next
task
    - A task uses the registers, RAM, and ROM of a microcontroller making up the 
    "Task Execuction Context"
    - A task does not know when it will be suspeneded and does not know when it happens
    - When a task is suspended the kernel saves its context and restores it when the task
    is brought back. This is called "Context Switching" and FreeRTOS handles it
- Task States
    - Running: Task is executing and using the processor
    - Ready: Tasks are able to execute but not currently executing
    because a task of equal or higher priority is in the running state
    - Blocked: Task is currently waiting either for a temporal or external event
    and is not using any processing time (ex: vTaskDelay just delays)
    - Suspended: Suspended state cannot be selected to enter the Running state, but
    tasks in the Suspended state do not have a time out. Use API calls vTaskSuspend() and
    xTaskResume() to enter and exit otherwise tasks cannot become or leave suspension
- Task Priorities
    - Priorities between 0-configMAX_PRIORITIES
    - Lower number = lower priority (0 is lowest)
    - FreeRTOS scheduler will always ensure that the highest
    priority task enters running state
    - Equal priority tasks will be entered in a nd out of running 
    state in round robin

- Higher Priority Tasks: 
    - The scheduler guarantees that the highest-priority task in the Ready state is 
    always the one running. It will not stop running for another task of the same priority unless it 
    calls a blocking function (like vTaskDelay() or reading a queue) or is preempted by a higher-priority task.
- Equal Priority Tasks: 
    - Time-slicing only applies when multiple tasks share the exact same highest priority. 
    In this case, tasks take turns using the CPU in a round-robin fashion.
Time Slice Duration: 
    - The length of a time slice for equal-priority tasks is determined by the system tick rate, 
    defined by the macro configTICK_RATE_HZ. If your tick rate is set to 1000 Hz (the default in many FreeRTOS 
    projects), the time slice is 1 ms.
        - The formula for the time slice duration in milliseconds is: (1000/configTICK_RATE_HZ) ms
*/

void task1(void *pvParameters) 
{   
    while (1){
        printf("Task 1 is currently running\n");
        // for(int i = 0; i < 20000000; i++){}
        vTaskDelay(pdMS_TO_TICKS(250));

    }
}

void task2(void *pvParameters)
{   
    while (1){
        printf("Task 2 is currently running\n");
        // for(int i = 0; i < 20000000; i++){}
        vTaskDelay(pdMS_TO_TICKS(250));
        /*
        For loop leads to a 1 sec delay and task 2 always
        runs due to its higher priority. However with the vTaskDelay
        places the respective task into the blocked state for 250 ticks,
        allowing the lower priority task 1 to run while task 2 is blocked

        For loop acts as a non-blocking delay while the vTaskDelay 
        is a blocking delay

        If the tasks were of equal priority, both tasks would run
        due to time slicing of tasks of equal priority
        */
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

    xTaskCreate(task1, "Task 1", 256, NULL, 1, NULL);
    xTaskCreate(task2, "Task 2", 256, NULL, 2, NULL); // Priority of 2
    vTaskStartScheduler();

    while(1){};
}