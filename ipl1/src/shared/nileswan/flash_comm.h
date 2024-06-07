/**
 * Copyright (c) 2024 Kemal Afzal
 *
 * Nileswan IPL1 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan IPL1 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan IPL1. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef __FLASH_COMM_H__
#define __FLASH_COMM_H__

#include <stdbool.h>
#include <stdint.h>

#include <wonderful.h>

void flash_read(uint32_t addr, uint16_t size, uint8_t __far* out);

#endif
