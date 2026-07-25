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

// ExecuTorch Runtime Includes
#include "executorch/extension/data_loader/buffer_data_loader.h"
#include "executorch/runtime/executor/program.h"
#include "executorch/runtime/platform/platform.h"
#include "executorch/runtime/core/exec_aten/exec_aten.h"

// Your model header containing: const unsigned char g_model_pte[] and g_model_pte_len
#include "gini_model.h" 

using namespace ::executorch::runtime;
using namespace ::executorch::extension;

// Define memory arena sizes (Tune these based on model memory planning profile)
#define INFERENCE_TASK_STACK_SIZE   (4 * 1024) / sizeof(StackType_t)
#define MEMORY_POOL_SIZE            (48 * 1024)

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

#define SENSOR_INPUT_COUNT 8
typedef float sens_package[SENSOR_INPUT_COUNT];
/*
0: soil_tempF
1: soil_tempC
2: soil_moisture
3: soil_ph
4: ambient_tempF
5: ambient_tempC
6: ambient_hum
7: lux
*/

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



// Planned memory pool for ExecuTorch tensor allocations
static uint8_t g_inference_pool[MEMORY_POOL_SIZE] __attribute__((aligned(16)));

// Dedicated FreeRTOS inference task routine
void vInferenceTask(void *pvParameters) {
    (void)pvParameters;
    printf("[ExecuTorch] Starting Inference Task...\n");

    // 1. Initialize ExecuTorch data loader with the embedded .pte array
    BufferDataLoader data_loader(models_model_ptes_gini_model_pte, models_model_ptes_gini_model_pte_len);

    // 2. Load the ExecuTorch Program structure
    Result<Program> program = Program::load(&data_loader);
    if (!program.ok()) {
        printf("[Error] Failed to load program. Code: %d\n", program.error());
        vTaskDelete(NULL);
    }

    // 3. Initialize Memory Manager
    /*
    A MemoryAllocator used to allocate runtime structures at Method load time. 
        Things like Tensor metadata, the internal chain of instructions, 
        and other runtime state come from this.
    */
    MemoryAllocator runtime_allocator(MEMORY_POOL_SIZE, g_inference_pool);
    MemoryManager memory_manager(&runtime_allocator);

    // 4. Load the compiled method profile (typically "forward")
    Result<Method> method = program->load_method("forward", &memory_manager);
    if (!method.ok()) {
        printf("[Error] Failed to load method 'forward'. Code: %d\n", method.error());
        vTaskDelete(NULL);
    }
    static sens_package receiving_package;


    printf("[ExecuTorch] Model initialized successfully. Running execution loop...\n");

    while (true) {
        // Fetch your hardware sensor inputs here ---
        // Example: float input_val = adc_read();
        xQueueRecieve(xQueue, &receiving_package, portMAX_DELAY); // Wait until data on a queue
        
        // Populate the model's input tensor buffers ---
        Result<void> set_res = method->set_input(0, &receiving_package, sizeof(receiving_package));

        if (set_res != Error::Ok) {
            printf("[Error] Failed to bind hardware memory to input tensor: %d\n", (int)set_res);
            // Handle error or skip this inference iteration
        } else {

        // Execute Inference ---
        uint32_t start_time = to_ms_since_boot(get_absolute_time());
        Error status = method->execute();
        uint32_t end_time = to_ms_since_boot(get_absolute_time());

        if (status == Error::Ok) {
            printf("Inference successful. Time elapsed: %lu ms\n", end_time - start_time);
            
            // Parse target output tensor ---
            exec_aten::Tensor output_tensor = method->get_output(0);
            float* output_data = output_tensor.data_ptr<float>();
        } else {
            printf("[Error] Inference failed with status code: %d\n", (int)status);
        }

        // Yield control to lower-priority FreeRTOS tasks for 100 milliseconds
        vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// Global initialization override needed by ExecuTorch runtime platform layers
extern "C" void et_pal_init(void) {}

void data_aquisition_task(void *pvParameters){
    dht_data current_readings;
    rs485_data rsdata;
    static sens_package reading_package;
    while (1) {
        /*
0: soil_tempF
1: soil_tempC
2: soil_moisture
3: soil_ph
4: ambient_tempF
5: ambient_tempC
6: ambient_hum
7: lux
*/
        if (read_from_dht(&current_readings)) {
            float fahrenheit = (current_readings.temperature * 9 / 5) + 32;
            reading_package[4] = fahrenheit;
            reading_package[6] = current_readings.humidity;
            reading_package[5] = current_readings.temperature;
        } else {
            printf("Failed to read data from DHT22 (Checksum/Timeout Error).\n");
        }
        readHumiturePH(&rsdata);
        float fahrenheit_rs = (rsdata.tem * 9 / 5) + 32;
        reading_package[1] = rsdata.tem;
        reading_package[0] = fahrenheit_rs;
        reading_package[2] = rsdata.hem;
        reading_package[3] = rsdata.ph;
        reading_package[7] = bh1750_read_light(I2C_PORT_BF1750) / 1.2;
        xQueueSend(xQueue, &reading_package, 0U);
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

    BaseType_t LED_task_status = xTaskCreate(
            led_task,       // Function pointer
            "LED_Task",    // Task name for debugging
            256,           // Stack depth
            NULL, // Parameter to pass (cast to void*)
            1,             // Priority
            NULL       // Task handle
    );

    BaseType_t FR_task_status = xTaskCreate(
        vInferenceTask,
        "ET_Inference",
        INFERENCE_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2, // High priority to prevent scheduler preemption during inference
        NULL
    );

    if (FR_task_status != pdPASS) {
        printf("[Fatal Error] FreeRTOS Task creation failed.\n");
        while(true);
    }
    if (LED_task_status != pdPASS) {
        printf("[Fatal Error] LED Task creation failed.\n");
        while(true);
    }

    vTaskStartScheduler();

    while(1){};
}