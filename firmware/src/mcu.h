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

// GPIO A
#define NILE_PORT_SPI GPIOA
#define NILE_PIN_MASK_SPI_NSS LL_GPIO_PIN_4
#define NILE_PIN_MASK_SPI_SCK LL_GPIO_PIN_5
#define NILE_PIN_MASK_SPI_POCI LL_GPIO_PIN_6
#define NILE_PIN_MASK_SPI_PICO LL_GPIO_PIN_7
#define NILE_PERIPH_SPI SPI1

// GPIO B
#define NILE_PIN_MASK_FPGA_IRQ LL_GPIO_PIN_6
#define NILE_PIN_MASK_SRAM_POWER LL_GPIO_PIN_7
#define NILE_PIN_MASK_USB_POWER LL_GPIO_PIN_8

#define NILE_SPI_FREQ_384KHZ LL_GPIO_SPEED_FREQ_LOW
#define NILE_SPI_FREQ_6MHZ   LL_GPIO_SPEED_FREQ_HIGH
#define NILE_SPI_FREQ_25MHZ  LL_GPIO_SPEED_FREQ_VERY_HIGH
void mcu_spi_set_freq(uint32_t freq);

void mcu_init(void);

bool mcu_usb_is_active(void);
void mcu_usb_power_task(void);

static inline void mcu_spi_enable(void) {
    LL_SPI_Enable(NILE_PERIPH_SPI);
}

static inline void mcu_spi_disable(void) {
    LL_SPI_Disable(NILE_PERIPH_SPI);
}

static inline bool mcu_usb_is_power_connected(void) {
    return LL_GPIO_IsInputPinSet(GPIOB, NILE_PIN_MASK_USB_POWER);
}

#endif /* _MCU_H_ */
