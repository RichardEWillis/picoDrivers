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

project(test_display_keypad C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable( test_display_keypad 
    ${CMAKE_CURRENT_LIST_DIR}/../../keyboard/keygpio/keygpio.c
    ${CMAKE_CURRENT_LIST_DIR}/../../display/displayBSP.c
    ${CMAKE_CURRENT_LIST_DIR}/../../display/common/cpyutils.c
    ${CMAKE_CURRENT_LIST_DIR}/../../display/common/textgfx.c
    ${CMAKE_CURRENT_LIST_DIR}/../../display/common/linegfx.c
    ${CMAKE_CURRENT_LIST_DIR}/../../display/ssd1309/ssd1309_driver.c
    test_display_keypad.c 
)

pico_set_program_name(test_display_keypad "test_display_keypad")
pico_set_program_version(test_display_keypad "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(test_display_keypad 1)
pico_enable_stdio_usb(test_display_keypad 0)

# Add the standard library to the build
target_link_libraries(test_display_keypad
    pico_stdlib
    hardware_spi
    pico_rand
    hardware_timer
)

# Add the standard include files to the build
target_include_directories(test_display_keypad PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../keyboard/include
    ${CMAKE_CURRENT_LIST_DIR}/../../display
    ${CMAKE_CURRENT_LIST_DIR}/../../display/include
)

# optimization control
#SET(CMAKE_C_FLAGS_DEBUG "-g -O0")
SET(CMAKE_C_FLAGS_RELEASE "-g -O0")

pico_add_extra_outputs(test_display_keypad)

