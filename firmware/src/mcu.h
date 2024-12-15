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

#include "config.h"

void mcu_init(void);

bool mcu_usb_is_active(void);
void mcu_usb_power_task(void);

static inline bool mcu_usb_is_power_connected(void) {
    return LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_USB_POWER);
}

#endif /* _MCU_H_ */
