cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(SD1000_cart C CXX ASM)


# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(SD1000_cart)

pico_define_boot_stage2(faster_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(faster_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
pico_set_boot_stage2(${PROJECT_NAME} faster_boot2)


# 16megs of flash on purple pico clones
target_compile_definitions(SD1000_cart PRIVATE
        PICO_STACK_SIZE=1024
        PICO_HEAP_SIZE=1024
        PICO_PANIC_FUNCTION=
        PICO_FLASH_SIZE_BYTES=15728640
)
add_compile_options(-flto -fwhole-program -ffunction-sections -fdata-sections -O3)

target_sources(SD1000_cart PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/sega_cart.c
)

target_include_directories(SD1000_cart PUBLIC
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(SD1000_cart 0)
pico_enable_stdio_uart(SD1000_cart 0)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support
target_link_libraries(SD1000_cart PUBLIC pico_runtime  )

# use custom linker script
pico_set_linker_script(SD1000_cart ${CMAKE_CURRENT_LIST_DIR}/memmap_sd1000.ld)


# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(SD1000_cart)
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage --data-sections --function-sections)