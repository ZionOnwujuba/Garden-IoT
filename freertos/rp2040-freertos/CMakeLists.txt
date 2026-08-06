# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico_w CACHE STRING "Board type")
set(PICO_CMAKE_CONFIG_INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src" 
    "${CMAKE_CURRENT_SOURCE_DIR}/lib" 
    CACHE PATH "Global Pico Config Dirs" FORCE)
include(pico_sdk_import.cmake)

cmake_minimum_required(VERSION 3.13)

# Enforce globally that NO libraries use Position Independent Code
set(CMAKE_POSITION_INDEPENDENT_CODE OFF CACHE BOOL "Disable PIC globally" FORCE)

# ExecuTorch compilation optimization toggles for embedded microcontrollers
set(EXECUTORCH_BUILD_EXECUTOR_RUNNER OFF CACHE BOOL "Disable heavy desktop runner binary" FORCE)
set(EXECUTORCH_BUILD_XNNPACK OFF CACHE BOOL "Disable XNNPACK for Pico" FORCE)
set(EXECUTORCH_USE_XNNPACK OFF CACHE BOOL "" FORCE)
set(EXECUTORCH_BUILD_CPUINFO OFF CACHE BOOL "Disable cpuinfo dependency entirely" FORCE)
set(EXECUTORCH_BUILD_PORTABLE_KERNELS ON CACHE BOOL "Enable Portable Kernels for Pico" FORCE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()


# Global target optimization flag (-Os shrinks size significantly)
set(CMAKE_C_FLAGS_RELEASE "-Os" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-Os" CACHE STRING "" FORCE)

project(rp2040-freertos C CXX ASM)

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)
set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "FreeRTOS Config Dir" FORCE)

include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Initialize Pico SDK BEFORE subdirectories so flags populate downward
pico_sdk_init()

set(FREERTOS_HEAP "HEAP_4" CACHE STRING "FreeRTOS Heap Management Strategy")

# Let the toolchain handle basic flags, but enforce code layout safety flags globally
add_compile_options(-mthumb -mcpu=cortex-m0plus -mfloat-abi=soft -fno-PIC -fno-PIE -fno-lto)
add_link_options(-mthumb -mcpu=cortex-m0plus -mfloat-abi=soft -fno-PIC -fno-PIE -fno-lto)

# Global dead-code elimination (strips unused methods from coreMQTT and mbedtls)
add_compile_options(-Os -fdata-sections -ffunction-sections)
add_link_options(-Wl,--gc-sections)

# Define processing subdirectories
add_subdirectory(lib/executorch EXECUTORCH_BUILD)
add_subdirectory(lib)
add_subdirectory(src)

# --- RESTRUCTURED TARGET POST-PROCESSING MACRO ---
macro(apply_no_pic_to_target target)
    if (TARGET ${target})
        get_target_property(target_type ${target} TYPE)
        
        if (target_type STREQUAL "EXECUTABLE" OR 
            target_type STREQUAL "STATIC_LIBRARY" OR 
            target_type STREQUAL "SHARED_LIBRARY" OR 
            target_type STREQUAL "MODULE_LIBRARY" OR 
            target_type STREQUAL "OBJECT_LIBRARY")
            
            set_target_properties(${target} PROPERTIES 
                POSITION_INDEPENDENT_CODE OFF
                INTERFACE_POSITION_INDEPENDENT_CODE OFF
            )
            target_compile_options(${target} PRIVATE -fno-PIC -fno-PIE -Os)

        elseif (target_type STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${target} INTERFACE -fno-PIC -fno-PIE)
        endif()
    endif()
endmacro()

function(process_all_targets dir)
    get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirs})
        process_all_targets(${subdir})
    endforeach()
    get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target ${targets})
        apply_no_pic_to_target(${target})
    endforeach()
endfunction()

# Trigger target processing AFTER all subdirectories are fully parsed into memory
process_all_targets(${CMAKE_CURRENT_SOURCE_DIR})
