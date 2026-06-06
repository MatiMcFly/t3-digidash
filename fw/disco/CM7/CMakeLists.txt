cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
set(STM32_MCU_FLAGS  "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard " )

#Linker options
set(STM32_LINKER_SCRIPT stm32h747xx_flash_CM7.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME t3_dash_CM7)
project(${CMAKE_PROJECT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated
include("mx-generated.cmake")

# The TouchGFX Generator emits target/generated/OSWrappers.cpp against
# CMSIS-RTOSv2 on every regen. This project uses native FreeRTOS, so drop
# the generated file from the project's source list and compile our own
# (see TouchGFX/target/OSWrappers_FreeRTOS.cpp) in its place.
get_target_property(_t3d_srcs ${CMAKE_PROJECT_NAME} SOURCES)
if(_t3d_srcs)
    list(FILTER _t3d_srcs EXCLUDE REGEX "TouchGFX/target/generated/OSWrappers\\.cpp$")
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY SOURCES ${_t3d_srcs})
endif()
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/target/OSWrappers_FreeRTOS.cpp
)

# Add touchgfx generated sources and libraries
add_subdirectory(cmake/touchgfx)


# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Core/Application
    # FreeRTOS kernel + port + project config
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/include
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1
    Core/Inc
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Application/App.c
    Core/Application/Backlight.c
    Core/Application/Display.c
    Core/Application/QspiFlash.c
    Core/Src/freertos_hooks.c
    Core/Src/axisram_bss_init.c
    # FreeRTOS kernel
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/tasks.c
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/queue.c
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/list.c
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/timers.c
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/event_groups.c
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/stream_buffer.c
    # FreeRTOS port (Cortex-M7 r0p1 errata-safe)
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1/port.c
    # FreeRTOS heap implementation
    ${CMAKE_SOURCE_DIR}/../Drivers/FreeRTOS-Kernel/portable/MemMang/heap_4.c
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add linked libraries
# NOTE: plain signature used to stay compatible with mx-generated.cmake
target_link_libraries(${CMAKE_PROJECT_NAME}
    # Add user defined libraries
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()









