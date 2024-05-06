cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)



# Pull in Raspberry Pi Pico SDK (must be before project)
# include(pico_sdk_import.cmake)
project(TDM_doubleBuffer_out C CXX ASM)
pico_sdk_init()

add_executable(TDM_doubleBuffer_out tdm.c sine_oscillator.c)

pico_generate_pio_header(TDM_doubleBuffer_out ${CMAKE_CURRENT_LIST_DIR}/tdm.pio)

target_sources(TDM_doubleBuffer_out PRIVATE TDM_doubleBuffer_out.c)

pico_set_program_name(TDM_doubleBuffer_out "TDM_doubleBuffer_out")
pico_set_program_version(TDM_doubleBuffer_out "1.0.0")



pico_enable_stdio_uart(TDM_doubleBuffer_out 0)
pico_enable_stdio_usb(TDM_doubleBuffer_out 1)

# Add the standard library to the build
target_link_libraries(TDM_doubleBuffer_out pico_stdlib)

# Add any user requested libraries
target_link_libraries(TDM_doubleBuffer_out
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_clocks
        )

pico_add_extra_outputs(TDM_doubleBuffer_out)
