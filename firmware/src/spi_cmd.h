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

#ifndef _SPI_CMD_H_
#define _SPI_CMD_H_

#include "spi.h"

typedef enum {
    MCU_SPI_CMD_ECHO = 0x00,
    MCU_SPI_CMD_MODE = 0x01,
    MCU_SPI_CMD_FREQ = 0x02,
    MCU_SPI_CMD_ID = 0x03,
    MCU_SPI_CMD_EEPROM_MODE = 0x10,
    MCU_SPI_CMD_EEPROM_ERASE = 0x11,
    MCU_SPI_CMD_EEPROM_READ = 0x12,
    MCU_SPI_CMD_EEPROM_WRITE = 0x13,
    MCU_SPI_CMD_RTC_COMMAND = 0x14,
    MCU_SPI_CMD_EEPROM_GET_MODE = 0x15,
    MCU_SPI_CMD_SET_SAVE_ID = 0x16,
    MCU_SPI_CMD_GET_SAVE_ID = 0x17,
    MCU_SPI_CMD_USB_CDC_READ = 0x40,
    MCU_SPI_CMD_USB_CDC_WRITE = 0x41,
    MCU_SPI_CMD_USB_HID_WRITE = 0x42,
    MCU_SPI_CMD_USB_CDC_AVAILABLE = 0x43,
} mcu_spi_cmd_t;

int spi_native_start_command_rx(uint16_t cmd);
int spi_native_finish_command_rx(uint8_t *rx, uint8_t *tx);

#endif /* _SPI_CMD_H_ */
