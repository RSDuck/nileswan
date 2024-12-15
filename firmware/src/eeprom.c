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

#include <string.h>
#include "mcu.h"
#include "eeprom.h"

static uint16_t eeprom_data[1024];
static uint16_t address_mask;
static uint8_t command_bits;
#ifdef CONFIG_FULL_EEPROM_EMULATION
static bool write_enabled;
#endif
static uint16_t write_command;

void eeprom_set_type(eeprom_type_t type) {
    write_command = 0;
#ifdef CONFIG_FULL_EEPROM_EMULATION
    write_enabled = false;
#endif

    switch (type) {
        case EEPROM_NONE:
            address_mask = 0;
            command_bits = 0;
            break;
        case EEPROM_M93LC06:
            address_mask = 0xF;
            command_bits = 4;
            break;
        case EEPROM_M93LC46:
            address_mask = 0x3F;
            command_bits = 4;
            break;
        case EEPROM_M93LC56:
            address_mask = 0x7F;
            command_bits = 6;
            break;
        case EEPROM_M93LC66:
            address_mask = 0xFF;
            command_bits = 6;
            break;
        case EEPROM_M93LC76:
            address_mask = 0x1FF;
            command_bits = 8;
            break;
        case EEPROM_M93LC86:
            address_mask = 0x3FF;
            command_bits = 8;
            break;
    }
}

void eeprom_erase(void) {
    memset(eeprom_data, 0xFF, 2048);
}

void eeprom_read_data(void *buffer, uint32_t address, uint32_t length) {
    memcpy(buffer, eeprom_data + address, length << 1);
}

void eeprom_write_data(const void *buffer, uint32_t address, uint32_t length) {
    memcpy(eeprom_data + address, buffer, length << 1);
}

uint16_t eeprom_exch_word(uint16_t w) {
    if (write_command) {
        uint8_t cmd = (write_command >> command_bits) & 0x1F;
#ifdef CONFIG_FULL_EEPROM_EMULATION
        if (write_enabled)
#endif
        {
            if (cmd == 0x11) {
                // WRAL
                for (int i = 0; i <= address_mask; i++)
                    eeprom_data[i] = w;
            } else {
                eeprom_data[write_command & address_mask] = w;
            }
        }
        write_command = 0;
        return 0xFFFF;
    }

    uint8_t cmd = (w >> command_bits) & 0x1F;
    switch (cmd) {
#ifdef CONFIG_FULL_EEPROM_EMULATION
    case 0x10: // WDS
        write_enabled = false;
        return 0xFFFF;
#endif
    case 0x12: // ERAL
        for (int i = 0; i <= address_mask; i++)
            eeprom_data[i] = 0xFFFF;
        return 0xFFFF;
#ifdef CONFIG_FULL_EEPROM_EMULATION
    case 0x13: // WEN
        write_enabled = true;
        return 0xFFFF;
#endif
    case 0x11: // WRAL
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17: // WRITE
        write_command = w;
        return 0xFFFF;
#ifdef CONFIG_FULL_EEPROM_EMULATION
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B: // READ
        return eeprom_data[w & address_mask];
#endif
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F: // ERASE
#ifdef CONFIG_FULL_EEPROM_EMULATION
        if (write_enabled)
#endif
            eeprom_data[w & address_mask] = 0xFFFF;
    default:
        return 0xFFFF;
    }
}
