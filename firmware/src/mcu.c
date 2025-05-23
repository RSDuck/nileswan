/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan MCU is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan MCU is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan MCU. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stm32u0xx_ll_gpio.h>
#include <stm32u0xx_ll_pwr.h>

#include "mcu.h"
#include "config.h"
#include "nvram.h"
#include "spi.h"
#include "tusb.h"

typedef enum {
    USB_INIT_STATUS_OFF = 0,
    USB_INIT_STATUS_REQUEST_ON = 1,
    USB_INIT_STATUS_ON = 2,
    USB_INIT_STATUS_REQUEST_OFF = 3
} usb_init_status_t;

static bool usb_enabled = false;
static uint8_t usb_init_status = USB_INIT_STATUS_OFF;

static void __mcu_usb_on_power_change(void) {
    if (mcu_usb_is_power_connected() && usb_enabled) {
        if (usb_init_status == USB_INIT_STATUS_OFF) {
            usb_init_status = USB_INIT_STATUS_REQUEST_ON;
        } else if (usb_init_status == USB_INIT_STATUS_ON) {
            USB->BCDR |= USB_BCDR_DPPU;
        }
    } else {
        if (usb_init_status == USB_INIT_STATUS_ON) {
            usb_init_status = USB_INIT_STATUS_REQUEST_OFF;
        }
    }
}

static void __mcu_bat_on_power_change(void) {
    // If battery inserted and running on battery, go to Standby
    if (LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_BAT) && LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_RUNS_ON_BAT)) {
        mcu_shutdown();
    }
}

void EXTI4_15_IRQHandler(void) {
    if ((EXTI->RPR1 & EXTI_RPR1_RPIF5) != 0) {
        EXTI->RPR1 = EXTI_RPR1_RPIF5;
        __mcu_usb_on_power_change();
    }

    if ((EXTI->FPR1 & EXTI_FPR1_FPIF5) != 0) {
        EXTI->FPR1 = EXTI_FPR1_FPIF5;
        __mcu_usb_on_power_change();
    }

    if ((EXTI->RPR1 & EXTI_RPR1_RPIF7) != 0) {
        EXTI->RPR1 = EXTI_RPR1_RPIF7;
        __mcu_bat_on_power_change();
    }

    if ((EXTI->FPR1 & EXTI_FPR1_FPIF7) != 0) {
        EXTI->FPR1 = EXTI_FPR1_FPIF7;
        __mcu_bat_on_power_change();
    }
}

void mcu_update_clock_speed(void) {
    uint32_t msi_range;
    uint32_t freq;

    msi_range = LL_RCC_MSIRANGE_8;
    freq = 16 * 1000 * 1000;

    // If SPI and USB don't require a 16MHz clock...
    if (usb_init_status == USB_INIT_STATUS_OFF && mcu_spi_get_freq() == MCU_SPI_FREQ_384KHZ) {
        if (mcu_spi_get_mode() == MCU_SPI_MODE_EEPROM || mcu_spi_get_mode() == MCU_SPI_MODE_CDC_OUTPUT) {
            // 1 MHz for slow EEPROM emulation
            // 1 MHz for USB output mode when USB not connected
            msi_range = LL_RCC_MSIRANGE_4;
            freq = 1 * 1000 * 1000;
        } else {
            // 8 MHz for non-USB mode
            msi_range = LL_RCC_MSIRANGE_7;
            freq = 8 * 1000 * 1000;
        }
    }

    // If USB debug is enabled...
    if (mcu_spi_get_freq() >= MCU_SPI_FREQ_6MHZ
#ifdef CONFIG_DEBUG_SPI_EEPROM_CMD
         || mcu_spi_get_mode() == MCU_SPI_MODE_EEPROM
#endif
#ifdef CONFIG_DEBUG_SPI_RTC_CMD
         || mcu_spi_get_mode() == MCU_SPI_MODE_RTC
#endif
    ) {
        if (mcu_usb_is_active()) {
            msi_range = LL_RCC_MSIRANGE_9;
            freq = 24 * 1000 * 1000;
        }
    }

    if (msi_range >= LL_RCC_MSIRANGE_5) {
        LL_PWR_DisableLowPowerRunMode();
        while (LL_PWR_IsEnabledLowPowerRunMode());

        if (msi_range >= LL_RCC_MSIRANGE_8) {
            LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
            while (LL_PWR_IsActiveFlag_VOS());
        }
    }

    while (!LL_RCC_MSI_IsReady());
    LL_RCC_MSI_SetRange(msi_range);

    if (msi_range < LL_RCC_MSIRANGE_8) {
        LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);

        if (msi_range < LL_RCC_MSIRANGE_5) {
            LL_PWR_EnableLowPowerRunMode();
        }
    }

    LL_SetSystemCoreClock(freq);
    LL_Init1msTick(freq);
}

void mcu_init(void) {
    LL_RCC_MSI_Enable();
    while (!LL_RCC_MSI_IsReady());

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_LPM_EnableSleep();

    LL_RCC_MSI_EnableRangeSelection();

    mcu_update_clock_speed();
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI);

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    
#ifdef TARGET_U0
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
#else
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
#endif

    LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
    LL_SYSTICK_EnableIT();

    LL_PWR_EnableBkUpAccess();

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB);

    // Initialize SPI
#ifdef TARGET_U0
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SPI1);
#else
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
#endif

    for (uint32_t pin = LL_GPIO_PIN_4; pin <= LL_GPIO_PIN_7; pin <<= 1) {
#ifdef TARGET_U0
        LL_GPIO_SetAFPin_0_7(MCU_PORT_SPI, pin, LL_GPIO_AF_5);
#else
        LL_GPIO_SetAFPin_0_7(MCU_PORT_SPI, pin, LL_GPIO_AF_0);
#endif
        LL_GPIO_SetPinOutputType(MCU_PORT_SPI, pin, LL_GPIO_OUTPUT_PUSHPULL);
        LL_GPIO_SetPinSpeed(MCU_PORT_SPI, pin, LL_GPIO_SPEED_FREQ_LOW);
        LL_GPIO_SetPinPull(MCU_PORT_SPI, pin, LL_GPIO_PULL_NO);
        LL_GPIO_SetPinMode(MCU_PORT_SPI, pin, LL_GPIO_MODE_ALTERNATE);
    }

    // Initialize FPGA pins
    LL_GPIO_SetPinMode(GPIOA, MCU_PIN_FPGA_IRQ, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(GPIOA, MCU_PIN_FPGA_IRQ, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOA, MCU_PIN_FPGA_IRQ, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinSpeed(GPIOA, MCU_PIN_FPGA_IRQ, LL_GPIO_SPEED_FREQ_LOW);
    mcu_fpga_irq_clear();

    // BUSY is only pulled high, never low
    LL_GPIO_SetPinMode(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_MODE_ANALOG);
    LL_GPIO_SetPinOutputType(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinSpeed(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetOutputPin(GPIOA, MCU_PIN_FPGA_BUSY);

    // Initialize VBUS sensing
    LL_GPIO_SetPinPull(GPIOB, MCU_PIN_USB_POWER, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinSpeed(GPIOB, MCU_PIN_USB_POWER, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinMode(GPIOB, MCU_PIN_USB_POWER, LL_GPIO_MODE_INPUT);

    // Initialize battery sensing
    // TODO: Set up a comparator for MCU_PIN_BAT
    LL_GPIO_SetPinPull(GPIOB, MCU_PIN_BAT, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinSpeed(GPIOB, MCU_PIN_BAT, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinMode(GPIOB, MCU_PIN_BAT, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOB, MCU_PIN_RUNS_ON_BAT, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinSpeed(GPIOB, MCU_PIN_RUNS_ON_BAT, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinMode(GPIOB, MCU_PIN_RUNS_ON_BAT, LL_GPIO_MODE_INPUT);

    // Keep the pulls active in Standby mode
    LL_PWR_EnableGPIOPullDown(LL_PWR_GPIO_B, MCU_PIN_USB_POWER);
    LL_PWR_EnableGPIOPullUp(LL_PWR_GPIO_B, MCU_PIN_RUNS_ON_BAT);
    LL_PWR_EnablePUPDCfg();

    LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE5);
    LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE7);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_5 | LL_EXTI_LINE_7);
    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_5 | LL_EXTI_LINE_7);
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_5 | LL_EXTI_LINE_7);

    NVIC_SetPriority(EXTI4_15_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(EXTI4_15_IRQn);

    NVIC_SetPriority(USB_DRD_FS_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));

    LL_mDelay(1);
    __mcu_bat_on_power_change();
    mcu_usb_set_enabled(true);

    while (!LL_PWR_IsEnabledBkUpAccess());
}

void mcu_usb_set_enabled(bool enabled) {
    usb_enabled = enabled;
    __mcu_usb_on_power_change();
}

bool mcu_usb_is_powered(void) {
    return usb_init_status == USB_INIT_STATUS_ON;
}

bool mcu_usb_is_active(void) {
    return usb_init_status == USB_INIT_STATUS_ON && tud_connected() && !tud_suspended();
}

void SysTick_Handler(void) {
}

void USB_DRD_FS_IRQHandler(void) {
    if (usb_init_status == USB_INIT_STATUS_ON) {
        tud_int_handler(0);
    }
}

static void __mcu_usb_power_on(void) {
    // Enable 48 MHz internal oscillator
    LL_RCC_HSI48_Enable();
    while (!LL_RCC_HSI48_IsReady());
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);

    // Enable clock recovery system
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_CRS);
    LL_CRS_ConfigSynchronization(LL_CRS_HSI48CALIBRATION_DEFAULT,
                                 LL_CRS_ERRORLIMIT_DEFAULT,
                                 LL_CRS_RELOADVALUE_DEFAULT,
                                 LL_CRS_SYNC_DIV_1 | LL_CRS_SYNC_SOURCE_USB | LL_CRS_SYNC_POLARITY_RISING);

    LL_CRS_EnableFreqErrorCounter();
    LL_CRS_EnableAutoTrimming();

    LL_PWR_EnableVddUSB();

    // Enable USB clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USB);
}

static void __mcu_usb_power_off(void) {
    LL_CRS_DisableAutoTrimming();
    LL_CRS_DisableFreqErrorCounter();

    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USB | LL_APB1_GRP1_PERIPH_CRS);
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL);
    LL_RCC_HSI48_Disable();

    LL_PWR_DisableVddUSB();
}

void mcu_usb_power_task(void) {
    if (usb_init_status == USB_INIT_STATUS_REQUEST_ON) {
        tusb_rhport_init_t dev_init = {
            .role = TUSB_ROLE_DEVICE,
            .speed = TUSB_SPEED_AUTO
        };

        __mcu_usb_power_on();
        mcu_update_clock_speed();
        tusb_init(0, &dev_init);
        usb_init_status = USB_INIT_STATUS_ON;

        __mcu_usb_on_power_change();
    } else if (usb_init_status == USB_INIT_STATUS_ON) {
        tud_task();
    } else if (usb_init_status == USB_INIT_STATUS_REQUEST_OFF) {
        USB->BCDR &= ~USB_BCDR_DPPU;

        __mcu_usb_power_off();

        usb_init_status = USB_INIT_STATUS_OFF;
        mcu_update_clock_speed();
    }
}

void mcu_shutdown(void) {
    LL_LPM_EnableDeepSleep();

    LL_PWR_DisableBkUpAccess();
    if (nvram_retention_required()) {
        LL_PWR_EnableSRAMRetention();
        LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
        while (!LL_PWR_IsEnabledSRAMRetention());
    } else {
        LL_PWR_DisableSRAMRetention();
        LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
        while (LL_PWR_IsEnabledSRAMRetention());
    }

    while (LL_PWR_IsEnabledBkUpAccess());
   
    if (usb_init_status != USB_INIT_STATUS_OFF) {
        usb_init_status = USB_INIT_STATUS_REQUEST_OFF;
        mcu_usb_power_task();
    }

    __disable_irq();

    while(1) __WFI();
}

void mcu_reset_backup_domain(void) {
    uint32_t save_id = TAMP->BKP8R;

    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();

    TAMP->BKP8R = save_id;
}
