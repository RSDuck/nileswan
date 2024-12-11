/**
 * Copyright (c) 2024 Adrian Siekierka
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>
#include <wonderful.h>

// util.s
void memcpy8to16(void *dst, const void *src, uint16_t count, uint16_t fill_value);
void print_hex_number(void *dst, uint16_t value);

#endif /* _UTIL_H_ */
