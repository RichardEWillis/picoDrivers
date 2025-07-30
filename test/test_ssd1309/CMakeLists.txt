# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(test_ssd1309 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# add definitions (defines) at the global scope
add_compile_definitions( 
    USE_FLOATING_TXTBUFS 
    USE_LINE_GFX 
    DISP_SSD1309
    LED_OVERLAY
)

# Add executable. Default name is the project name, version 0.1

add_executable( test_ssd1309 
    ${CMAKE_CURRENT_LIST_DIR}/../../display/ssd1309/ssd1309_driver.c
    ${CMAKE_CURRENT_LIST_DIR}/../../display/ssd1309/ssd1309_textbuffer.c
    ${CMAKE_CURRENT_LIST_DIR}/../../display/ssd1309/led_overlay.c
    test_ssd1309.c 
)

pico_set_program_name(test_ssd1309 "test_ssd1309")
pico_set_program_version(test_ssd1309 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(test_ssd1309 0)
pico_enable_stdio_usb(test_ssd1309 0)

# Add the standard library to the build
target_link_libraries(test_ssd1309
    pico_stdlib
    hardware_spi
    pico_rand
)

# Add the standard include files to the build
target_include_directories(test_ssd1309 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../display/include
)

# optimization control
SET(CMAKE_C_FLAGS_DEBUG "-g -O0")
#SET(CMAKE_C_FLAGS_RELEASE "-O2")
SET(CMAKE_C_FLAGS_RELEASE "-g-O0")


pico_add_extra_outputs(test_ssd1309)

