add_executable( 
rp2040-freertos main.c
)

set_target_properties(rp2040-freertos PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(${ProjectName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${ProjectName} 
    pico_stdlib 
    FreeRTOS-Kernel-Heap4
    pico_cyw43_arch_none # We use _none because you are just blinking an LED. 
                         # Use pico_cyw43_arch_lwip_threadsafe_background later 
                         # if you eventually add actual Wi-Fi networking with FreeRTOS
    pico_multicore
    hardware_i2c
    pico_binary_info
    hardware_uart
    )

pico_enable_stdio_usb(rp2040-freertos 1) # Enable usb output
pico_enable_stdio_uart(rp2040-freertos 0) # disable uart output


pico_add_extra_outputs(${ProjectName})