set(core_mqtt_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/core_mqtt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/core_mqtt_serializer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/core_mqtt_serializer_private.c
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/core_mqtt_prop_serializer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/core_mqtt_prop_deserializer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/core_mqtt_state.c
)

# Explicitly force C compilation rules
set_source_files_properties(${core_mqtt_sources} PROPERTIES LANGUAGE C)

add_library(coremqtt_lib STATIC ${core_mqtt_sources})

# PUBLIC allows targets linking coremqtt_lib to automatically inherit these include paths
target_include_directories(coremqtt_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/coreMQTT/source/interface
)

target_link_libraries(coremqtt_lib PUBLIC
    pico_stdlib 
)

# Force proper ARM architecture settings on coreMQTT compilation
target_compile_options(coremqtt_lib PRIVATE "-mthumb" "-mcpu=cortex-m0plus" "-fno-lto")



add_library(peripheral_drivers STATIC
    peripheral-libs/dht22.c
    peripheral-libs/dht22.h
    peripheral-libs/lcd.c
    peripheral-libs/lcd.h
    peripheral-libs/light_sensor.c
    peripheral-libs/light_sensor.h
    peripheral-libs/rs485.c
    peripheral-libs/rs485.h
    key_certs.h
    constants.h
    gini_model.h
)

# Explicitly clean up include paths
target_include_directories(peripheral_drivers PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/peripheral-libs
    ${CMAKE_CURRENT_SOURCE_DIR}/executorch
    ${CMAKE_CURRENT_SOURCE_DIR}/executorch/extension/runner/out/include
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/include
)

# Link hardware APIs, FreeRTOS, and let it inherit coremqtt_lib automatically
target_link_libraries(peripheral_drivers PUBLIC
    pico_stdlib 
    pico_cyw43_arch_lwip_sys_freertos 
    pico_multicore
    hardware_i2c
    pico_binary_info
    hardware_uart
    hardware_dma 
    FreeRTOS-Kernel-Heap4
    coremqtt_lib  
)

# Enforce uniform bare-metal safety properties
target_compile_options(peripheral_drivers PRIVATE "-mthumb" "-mcpu=cortex-m0plus" "-fno-lto")
