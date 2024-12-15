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

#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "mcu.h"

typedef enum {
    EEPROM_NONE,
    EEPROM_M93LC06,
    EEPROM_M93LC46,
    EEPROM_M93LC56,
    EEPROM_M93LC66,
    EEPROM_M93LC76,
    EEPROM_M93LC86
} eeprom_type_t;

void eeprom_erase(void);
void eeprom_read_data(void *buffer, uint32_t address, uint32_t length);
void eeprom_write_data(const void *buffer, uint32_t address, uint32_t length);
void eeprom_set_type(eeprom_type_t type);
uint16_t eeprom_exch_word(uint16_t w);

#endif /* _EEPROM_H_ */
