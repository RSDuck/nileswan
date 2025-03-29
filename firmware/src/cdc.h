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

#ifndef _CDC_H_
#define _CDC_H_

#include "mcu.h"

#ifdef CONFIG_ENABLE_CDC_DEBUG_PORT
int cdc_debug(const char *format, ...);
void cdc_debug_write(void *data, int len);
void cdc_debug_write_hex16(uint16_t v);
#else
#define cdc_debug(...)
#define cdc_debug_write(...)
#define cdc_debug_write_hex16(...)
#endif

#endif /* _CDC_H_ */
