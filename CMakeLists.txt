cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_PATH "../../pico-sdk")
include(pico_sdk_import.cmake)

project(pico_motor_project C CXX ASM)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(PROJECT vga)
set(USB_TOP ${PICO_SDK_PATH}/lib/tinyusb/)

add_executable(${PROJECT})

target_sources(${PROJECT} PUBLIC
    main.cpp gterm.cpp states.cpp vt52_states.cpp actions.cpp utils.cpp vgaterm.cpp hid_app.cpp mouse.cpp video.cpp console.cpp graphics.cpp console_stdio.cpp
    lisp.cpp lisp_operators.cpp lisp_parser.cpp #msc_app.cpp
    ${USB_TOP}/lib/fatfs/source/ff.c
    ${USB_TOP}/lib/fatfs/source/ffsystem.c
    ${USB_TOP}/lib/fatfs/source/ffunicode.c
    )

target_include_directories(${PROJECT} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${USB_TOP}/lib/fatfs/source
        ${USB_TOP}/lib/embedded-cli
        )

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(vga ${CMAKE_CURRENT_LIST_DIR}/pio-vga.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

pico_enable_stdio_usb(vga 0)
pico_enable_stdio_uart(vga 1)

# Add extra flags for the specific target (RP2040 in this case)
pico_add_extra_outputs(vga)

# Link the Pico SDK to your executable
target_link_libraries(vga PRIVATE pico_stdlib hardware_pio hardware_dma pico_multicore tinyusb_host tinyusb_board)
