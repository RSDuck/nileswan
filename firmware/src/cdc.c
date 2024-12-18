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

#include <stdarg.h>
#include "nanoprintf.h"

#include "mcu.h"
#include "tusb.h"

#ifdef CONFIG_ENABLE_CDC_DEBUG_PORT
int cdc_debug(const char *format, ...) {
    char buf[96];
    va_list val;
    va_start(val, format);
    int n = npf_vsnprintf(buf, sizeof(buf), format, val);
    tud_cdc_n_write_str(1, buf);
    tud_cdc_n_write_flush(1);
    va_end(val);
    return n;
}
#endif
