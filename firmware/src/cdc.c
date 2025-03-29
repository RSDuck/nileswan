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
    if (!mcu_usb_is_active()) return 0;
    char buf[96];
    va_list val;
    va_start(val, format);
    int n = npf_vsnprintf(buf, sizeof(buf), format, val);
    tud_cdc_n_write_str(1, buf);
    va_end(val);
    return n;
}

void cdc_debug_write(void *data, int len) {
    if (!mcu_usb_is_active()) return;
    tud_cdc_n_write(1, data, len);
}

static const char hexchars[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void cdc_debug_write_hex16(uint16_t v) {
    char c[6];
    c[0] = hexchars[v >> 12];
    c[1] = hexchars[(v >> 8) & 0xF];
    c[2] = hexchars[(v >> 4) & 0xF];
    c[3] = hexchars[v & 0xF];
    c[4] = 13;
    c[5] = 10;
    cdc_debug_write(c, sizeof(c));
}
#endif
