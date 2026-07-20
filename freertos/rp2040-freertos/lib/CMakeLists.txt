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
)


target_include_directories(peripheral_drivers PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/peripheral-libs
)
target_link_libraries(peripheral_drivers PRIVATE
    pico_stdlib 
    pico_cyw43_arch_none # We use _none because you are just blinking an LED. 
                         # Use pico_cyw43_arch_lwip_threadsafe_background later 
                         # if you eventually add actual Wi-Fi networking with FreeRTOS
    pico_multicore
    hardware_i2c
    pico_binary_info
    hardware_uart
    )