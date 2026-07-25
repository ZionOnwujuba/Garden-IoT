add_library(peripheral_drivers STATIC
    peripheral-libs/dht22.c
    peripheral-libs/dht22.h
    peripheral-libs/lcd.c
    peripheral-libs/lcd.h
    peripheral-libs/light_sensor.c
    peripheral-libs/light_sensor.h
    peripheral-libs/rs485.c
    peripheral-libs/rs485.h
    constants.h
    gini_model.h
)

set(EXECUTORCH_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/executorch")
set(EXECUTORCH_BUILD_XNNPACK OFF CACHE BOOL "Disable XNNPACK for Pico" FORCE)
set(EXECUTORCH_BUILD_VULKAN OFF CACHE BOOL "Disable Vulkan for Pico" FORCE)
set(EXECUTORCH_BUILD_MPS OFF CACHE BOOL "Disable MPS for Pico" FORCE)
set(EXECUTORCH_BUILD_COREML OFF CACHE BOOL "Disable CoreML for Pico" FORCE)
set(EXECUTORCH_BUILD_OPTIMIZED_KERNELS OFF CACHE BOOL "Disable Optimized Kernels" FORCE)
set(EXECUTORCH_BUILD_PORTABLE_KERNELS ON CACHE BOOL "Enable Portable Kernels for Pico" FORCE)

target_include_directories(peripheral_drivers PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/peripheral-libs
    ${EXECUTORCH_ROOT_DIR}
    ${EXECUTORCH_ROOT_DIR}/extension/runner/out/include
)

add_subdirectory("executorch")

target_link_libraries(peripheral_drivers PRIVATE
    pico_stdlib 
    pico_cyw43_arch_none # We use _none because you are just blinking an LED. 
                         # Use pico_cyw43_arch_lwip_threadsafe_background later 
                         # if you eventually add actual Wi-Fi networking with FreeRTOS
    pico_multicore
    hardware_i2c
    pico_binary_info
    hardware_uart
    executorch
    executorch::backends
    executorch::extensions
    executorch::kernels
    )