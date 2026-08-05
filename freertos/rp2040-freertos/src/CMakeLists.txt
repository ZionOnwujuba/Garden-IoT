cmake_minimum_required(VERSION 3.13)

add_executable(rp2040-freertos main.cpp)

pico_set_program_name(rp2040-freertos "rp2040-freertos")

set_target_properties(rp2040-freertos PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_compile_options(rp2040-freertos PRIVATE 
    -Os 
    -mthumb 
    -mcpu=cortex-m0plus
    -fno-lto
)

target_link_options(rp2040-freertos PRIVATE 
    -mthumb 
    -mcpu=cortex-m0plus
    -fno-lto
)

target_compile_definitions(rp2040-freertos PRIVATE 
    PICO_USE_THREAD_LOCAL=0
    MBEDTLS_CONFIG_FILE="mbedtls_config.h"
)

target_include_directories(rp2040-freertos PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_SOURCE_DIR}/src                     
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/coreMQTT/source/include
    ${CMAKE_SOURCE_DIR}/lib/coreMQTT/source/interface
)

target_link_libraries(rp2040-freertos PRIVATE
    # SDK Base & Multicore Core
    pico_stdlib 
    pico_multicore
    pico_binary_info
    pico_rand
    
    # Hardware Driver APIs
    hardware_i2c
    hardware_uart
    hardware_rosc
    hardware_dma
    
    # Static App Libraries 
    peripheral_drivers
    coremqtt_lib
    
    # Network Layer & FreeRTOS Stack Integration
    FreeRTOS-Kernel-Heap4
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_mbedtls
    pico_mbedtls
    pico_lwip_sntp

    # ExecuTorch Runtime & Data Loader Extensions
    executorch
    extension_data_loader
    
    # Force GNU Linker to embed all operators inside the target binary
    -Wl,--whole-archive
    portable_ops_lib
    -Wl,--no-whole-archive
)

pico_enable_stdio_usb(rp2040-freertos 1) 
pico_enable_stdio_uart(rp2040-freertos 0) 

pico_add_extra_outputs(rp2040-freertos)
