cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake/stm32_gcc.cmake)

project(firmware C ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS "-g -Os -mcpu=cortex-m0plus -mthumb")

stm32_fetch_cmsis(L0)
stm32_fetch_hal(L0)

find_package(CMSIS COMPONENTS STM32L052K8 REQUIRED)
find_package(HAL COMPONENTS STM32L0 LL_RCC LL_EXTI LL_GPIO LL_SPI LL_UTILS REQUIRED)

add_library(tinyusb_config INTERFACE)
target_include_directories(tinyusb_config INTERFACE src)
target_compile_definitions(tinyusb_config INTERFACE CFG_TUSB_MCU=OPT_MCU_STM32L0)
add_subdirectory(tinyusb/src)

set(PROJECT_SOURCES
    src/main.c
    src/mcu.c
    src/usb_descriptors.c
    tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
)

add_executable(firmware ${PROJECT_SOURCES})
target_include_directories(firmware PRIVATE src)
target_link_libraries(firmware
    HAL::STM32::L0::LL_RCC
    HAL::STM32::L0::LL_EXTI
    HAL::STM32::L0::LL_GPIO
    HAL::STM32::L0::LL_SPI
    HAL::STM32::L0::LL_UTILS
    CMSIS::STM32::L052K8
    STM32::NoSys
    tinyusb
)
stm32_print_size_of_target(firmware)
stm32_generate_binary_file(firmware)
