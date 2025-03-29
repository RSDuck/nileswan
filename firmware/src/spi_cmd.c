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

#include "class/cdc/cdc_device.h"
#include "mcu.h"
#include "cdc.h"
#include "nvram.h"
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
    case MCU_SPI_CMD_EEPROM_WRITE:
        return (arg_to_len(arg) << 1) + 2;
    case MCU_SPI_CMD_EEPROM_READ:
        return 2;
    case MCU_SPI_CMD_SET_SAVE_ID:
        return 4;
    case MCU_SPI_CMD_RTC_COMMAND:
        return rtc_start_command_rx(arg);
    default:
        return 0;
    }
}

int spi_native_finish_command_rx(uint8_t *rx, uint8_t *tx) {
    uint16_t arg = SPI_NATIVE_ARG(spi_cmd);
#ifdef CONFIG_DEBUG_SPI_NATIVE_CMD
    cdc_debug("spi/native: received command %02X %04X", SPI_NATIVE_CMD(spi_cmd), arg);
#endif
    switch (SPI_NATIVE_CMD(spi_cmd)) {
    case MCU_SPI_CMD_ECHO:
        memcpy(tx, rx, arg_to_len(arg));
        return arg_to_len(arg);
    case MCU_SPI_CMD_MODE:
        if (arg == 0xFF) {
            mcu_shutdown();
        } else {
            mcu_spi_init(arg);
        }
        return -1;
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
    case MCU_SPI_CMD_EEPROM_ERASE:
        eeprom_erase();
        return 0;
    case MCU_SPI_CMD_EEPROM_WRITE:
        eeprom_write_data(rx + 2, *((uint16_t*) rx), arg_to_len(arg) << 1);
        return 0;
    case MCU_SPI_CMD_EEPROM_READ:
        eeprom_read_data(tx, *((uint16_t*) rx), arg_to_len(arg) << 1);
        return arg_to_len(arg) << 1;
    case MCU_SPI_CMD_RTC_COMMAND:
        return rtc_finish_command_rx(rx, tx);
    case MCU_SPI_CMD_EEPROM_GET_MODE:
        tx[0] = eeprom_get_type();
        return 1;
    case MCU_SPI_CMD_SET_SAVE_ID:
        memcpy(&nvram.save_id, rx, 4);
        tx[0] = 1;
        return 1;
    case MCU_SPI_CMD_GET_SAVE_ID:
        memcpy(tx, &nvram.save_id, 4);
        return 4;
    case MCU_SPI_CMD_USB_CDC_READ:
        if (!mcu_usb_is_powered()) {
            return 0;
        }
        return tud_cdc_read(tx, arg_to_len(arg));
    case MCU_SPI_CMD_USB_CDC_WRITE: {
        if (!mcu_usb_is_powered()) {
            tx[0] = 0;
            tx[1] = 0;
        } else {
            uint16_t v16 = tud_cdc_write(rx, arg_to_len(arg));
            tx[0] = v16;
            tx[1] = v16 >> 8;
            tud_cdc_write_flush();
        }
        return 2;
    }
    case MCU_SPI_CMD_USB_CDC_AVAILABLE: {
        if (!mcu_usb_is_powered()) {
            tx[0] = 0;
            tx[1] = 0;
        } else {
            uint16_t v16 = tud_cdc_available();
            tx[0] = v16;
            tx[1] = v16 >> 8;
        }
        return 2;
    }
    default:
        return 0;
    }
}

#if 0
void tud_cdc_rx_cb(uint8_t itf) {
    if (itf == 0 && tud_cdc_n_available(itf)) {
        cdc_debug("available: %d\n", tud_cdc_n_available(itf));
    }
}
#endif
