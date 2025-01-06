cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake/stm32_gcc.cmake)

option(TARGET_U0 "Target U0 family" OFF)

project(firmware C ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")

if(TARGET_U0)
    stm32_fetch_cmsis(U0)
    stm32_fetch_hal(U0)

    find_package(CMSIS COMPONENTS STM32U073KC REQUIRED)
    find_package(HAL COMPONENTS STM32U0 LL_RCC LL_EXTI LL_GPIO LL_RTC LL_SPI LL_UTILS REQUIRED)

    set(CMAKE_C_FLAGS "-g -Os -mcpu=cortex-m0plus -mthumb -DTARGET_U0")
else()
    stm32_fetch_cmsis(L0)
    stm32_fetch_hal(L0)

    find_package(CMSIS COMPONENTS STM32L052K8 REQUIRED)
    find_package(HAL COMPONENTS STM32L0 LL_RCC LL_EXTI LL_GPIO LL_RTC LL_SPI LL_UTILS REQUIRED)

    set(CMAKE_C_FLAGS "-g -Os -mcpu=cortex-m0plus -mthumb -DTARGET_L0")
endif()

set(PROJECT_SOURCES
    src/cdc.c
    src/eeprom.c
    src/hid.c
    src/main.c
    src/mcu.c
    src/nanoprintf.c
    src/nvram.c
    src/rtc.c
    src/spi.c
    src/spi_cmd.c
    src/usb_descriptors.c

    # TinyUSB common
    tinyusb/src/tusb.c
    tinyusb/src/common/tusb_fifo.c
    # TinyUSB device
    tinyusb/src/device/usbd.c
    tinyusb/src/device/usbd_control.c
    tinyusb/src/class/cdc/cdc_device.c
    tinyusb/src/class/hid/hid_device.c
    tinyusb/src/class/msc/msc_device.c
    tinyusb/src/class/net/ecm_rndis_device.c
    tinyusb/src/class/net/ncm_device.c
    tinyusb/src/class/vendor/vendor_device.c
    # TinyUSB driver
    tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
)

add_executable(firmware ${PROJECT_SOURCES})
target_include_directories(firmware PRIVATE src tinyusb/src)
if(TARGET_U0)
    target_compile_definitions(firmware PRIVATE CFG_TUSB_MCU=OPT_MCU_STM32U0)
    target_link_libraries(firmware
        HAL::STM32::U0::LL_RCC
        HAL::STM32::U0::LL_EXTI
        HAL::STM32::U0::LL_GPIO
        HAL::STM32::U0::LL_RTC
        HAL::STM32::U0::LL_SPI
        HAL::STM32::U0::LL_UTILS
        CMSIS::STM32::U073xx
        STM32::Nano
    )
    stm32_add_linker_script(firmware PRIVATE src/link_U073KC.ld)
else()
    target_compile_definitions(firmware PRIVATE CFG_TUSB_MCU=OPT_MCU_STM32L0)
    target_link_libraries(firmware
        HAL::STM32::L0::LL_RCC
        HAL::STM32::L0::LL_EXTI
        HAL::STM32::L0::LL_GPIO
        HAL::STM32::L0::LL_RTC
        HAL::STM32::L0::LL_SPI
        HAL::STM32::L0::LL_UTILS
        CMSIS::STM32::L052xx
        STM32::Nano
    )
    stm32_add_linker_script(firmware PRIVATE src/link_L052K8.ld)
endif()
stm32_print_size_of_target(firmware)
stm32_generate_binary_file(firmware)
