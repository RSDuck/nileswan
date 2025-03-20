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

#ifndef _CONFIG_H_
#define _CONFIG_H_

// Feature flags

// Enable full EEPROM emulation.
// If disabled, READ and WDS/WEN commands are ignored (emulated on FPGA side only).
// #define CONFIG_FULL_EEPROM_EMULATION

#define CONFIG_ENABLE_CDC_DEBUG_PORT

// Firmware configuration

#define MCU_SPI_MAX_PACKET_SIZE 512
#define MCU_SPI_RX_BUFFER_SIZE (MCU_SPI_MAX_PACKET_SIZE)
#define MCU_SPI_TX_BUFFER_SIZE ((MCU_SPI_MAX_PACKET_SIZE) + 2)

// MCU configuration

#define MCU_UID_LENGTH 12

// Debug configuration

#define CONFIG_DEBUG_SPI_NATIVE_CMD
#define CONFIG_DEBUG_SPI_EEPROM_CMD
#define CONFIG_DEBUG_SPI_RTC_CMD

// GPIO A
#define MCU_PORT_SPI GPIOA
#define MCU_PIN_SPI_NSS LL_GPIO_PIN_4
#define MCU_PIN_SPI_SCK LL_GPIO_PIN_5
#define MCU_PIN_SPI_POCI LL_GPIO_PIN_6
#define MCU_PIN_SPI_PICO LL_GPIO_PIN_7
#define MCU_PERIPH_SPI SPI1

// GPIO B
#define MCU_PIN_FPGA_IRQ LL_GPIO_PIN_6
#define MCU_PIN_SRAM_POWER LL_GPIO_PIN_7
#ifdef TARGET_U0
#define MCU_PIN_USB_POWER LL_GPIO_PIN_5
#else
#define MCU_PIN_USB_POWER LL_GPIO_PIN_8
#endif

// TODO: In rev6+, this is a separate pin
#define MCU_PIN_FPGA_READY LL_GPIO_PIN_6

#define MCU_DMA_CHANNEL_SPI_RX LL_DMA_CHANNEL_2
#define MCU_DMA_CHANNEL_SPI_TX LL_DMA_CHANNEL_3

#define MCU_SPI_FREQ_384KHZ LL_GPIO_SPEED_FREQ_LOW
#define MCU_SPI_FREQ_6MHZ   LL_GPIO_SPEED_FREQ_HIGH
#define MCU_SPI_FREQ_25MHZ  LL_GPIO_SPEED_FREQ_VERY_HIGH

#endif /* _CONFIG_H_ */
