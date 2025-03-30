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

#ifndef _MCU_H_
#define _MCU_H_

#include <stdbool.h>

#ifdef TARGET_U0
#include <stm32u073xx.h>
#include <stm32u0xx_ll_rcc.h>
#include <stm32u0xx_ll_bus.h>
#include <stm32u0xx_ll_exti.h>
#include <stm32u0xx_ll_system.h>
#include <stm32u0xx_ll_cortex.h>
#include <stm32u0xx_ll_utils.h>
#include <stm32u0xx_ll_gpio.h>
#include <stm32u0xx_ll_spi.h>
#include <stm32u0xx_ll_pwr.h>
#include <stm32u0xx_ll_usb.h>
#include <stm32u0xx_ll_crs.h>
#include <stm32u0xx_ll_dma.h>
#include <stm32u0xx_ll_rtc.h>

#define USB USB_DRD_FS
#define LL_RCC_USB_CLKSOURCE_PLL LL_RCC_USB_CLKSOURCE_PLLQ
#define RTC_IRQn RTC_TAMP_IRQn
#else
#include <stm32l052xx.h>
#include <stm32l0xx_ll_rcc.h>
#include <stm32l0xx_ll_bus.h>
#include <stm32l0xx_ll_exti.h>
#include <stm32l0xx_ll_system.h>
#include <stm32l0xx_ll_cortex.h>
#include <stm32l0xx_ll_utils.h>
#include <stm32l0xx_ll_gpio.h>
#include <stm32l0xx_ll_spi.h>
#include <stm32l0xx_ll_pwr.h>
#include <stm32l0xx_ll_usb.h>
#include <stm32l0xx_ll_crs.h>
#include <stm32l0xx_ll_dma.h>
#include <stm32l0xx_ll_rtc.h>
#endif

#include "config.h"

void mcu_init(void);
void mcu_update_clock_speed(void);
void mcu_shutdown(void);
void mcu_usb_power_task(void);

/**
 * @brief Signal to the FPGA that the MCU is currently busy. 
 */
static inline void mcu_fpga_start_busy(void) {
    LL_GPIO_SetPinMode(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_MODE_OUTPUT);
}
/**
 * @brief Signal to the FPGA that the MCU is no longer busy. 
 */
static inline void mcu_fpga_finish_busy(void) {
    LL_GPIO_SetPinMode(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_MODE_ANALOG);
}

/**
 * @brief Set whether or not the USB port should be enabled (on the MCU side).
 * The USB hardware will only be activated if the USB port is enabled and an USB
 * cable is physically connected to a host.
 */
void mcu_usb_set_enabled(bool enabled);
/**
 * @brief Returns true if the USB port is powered on and enabled.
 */
bool mcu_usb_is_powered(void);
/**
 * @brief Returns true if the USB port is powered on and enabled,
 * and the USB device is active (connected, not suspended).
 */
bool mcu_usb_is_active(void);

/**
 * @brief Returns true if the USB port is powered on (physically).
 */
static inline bool mcu_usb_is_power_connected(void) {
    return LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_USB_POWER);
}

#endif /* _MCU_H_ */
