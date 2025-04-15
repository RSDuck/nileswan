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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "eeprom.h"
#include "nvram.h"

__attribute__((section(".nvram")))
nvram_t nvram;

void nvram_init(void) {
    if (nvram.magic != NVRAM_MAGIC) {
        memset(&nvram, 0, sizeof(nvram));
        nvram.magic = NVRAM_MAGIC;
        nvram.save_id = SAVE_ID_NONE;
    }

    eeprom_set_type(nvram.eeprom_type);
}

bool nvram_retention_required(void) {
    return (nvram.eeprom_type != EEPROM_NONE && nvram.save_id != SAVE_ID_NONE);
}
