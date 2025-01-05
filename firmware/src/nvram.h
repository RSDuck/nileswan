/**
 * Copyright (c) 2025 Adrian Siekierka
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

#ifndef _NVRAM_H_
#define _NVRAM_H_

#include "mcu.h"

#define NVRAM_MAGIC 0xAA557113

typedef struct {
    uint32_t magic;
    uint16_t eeprom_data[1024];
    uint8_t eeprom_type;
} nvram_t;

extern nvram_t nvram;
void nvram_init(void);

#endif /* _NVRAM_H_ */
