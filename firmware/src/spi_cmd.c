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
#include <string.h>

#include "mcu.h"
#include "spi_cmd.h"
#include "eeprom.h"
#include "rtc.h"
#include "tusb.h"

uint16_t spi_cmd;
#define SPI_NATIVE_CMD(n) ((n) & 0x7F)
#define SPI_NATIVE_ARG(n) ((n) >> 7)

static inline int arg_to_len(uint16_t arg) {
    return arg ? arg : 512;
}

int spi_native_start_command_rx(uint16_t cmd) {
    spi_cmd = cmd;
    uint16_t arg = SPI_NATIVE_ARG(spi_cmd);
    switch (SPI_NATIVE_CMD(spi_cmd)) {
    case MCU_SPI_CMD_ECHO:
    case MCU_SPI_CMD_USB_CDC_WRITE:
        return arg_to_len(arg);
    case MCU_SPI_CMD_RTC_COMMAND:
        return rtc_start_command_rx(arg);
    default:
        return 0;
    }
}

int spi_native_finish_command_rx(uint8_t *rx, uint8_t *tx) {
    uint16_t arg = SPI_NATIVE_ARG(spi_cmd);
    switch (SPI_NATIVE_CMD(spi_cmd)) {
    case MCU_SPI_CMD_ECHO:
        memcpy(tx, rx, arg_to_len(arg));
        return arg_to_len(arg);
    case MCU_SPI_CMD_MODE:
        mcu_spi_init(arg);
        return 0;
    case MCU_SPI_CMD_FREQ:
        mcu_spi_set_freq(arg);
        tx[0] = 1;
        return 1;
    case MCU_SPI_CMD_ID:
        memcpy(tx, (void*) UID_BASE, MCU_UID_LENGTH);
        return MCU_UID_LENGTH;
    case MCU_SPI_CMD_EEPROM_MODE:
        eeprom_set_type(arg);
        tx[0] = 1;
        return 1;
    case MCU_SPI_CMD_RTC_COMMAND:
        return rtc_finish_command_rx(rx, tx);
    case MCU_SPI_CMD_USB_CDC_READ:
        if (!mcu_usb_is_active()) {
            return 0;
        }
        return tud_cdc_read(tx, arg_to_len(arg));
    case MCU_SPI_CMD_USB_CDC_WRITE: {
        if (!mcu_usb_is_active()) {
            tx[0] = 0;
            tx[1] = 0;
        } else {
            int cdc_write_len = tud_cdc_write(rx, arg_to_len(arg));
            tx[0] = cdc_write_len;
            tx[1] = cdc_write_len >> 8;
            tud_cdc_write_flush();
        }
        return 2;
    }
    default:
        return 0;
    }
}