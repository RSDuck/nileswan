/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan Updater is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan Updater is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan Updater. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <wsx/zx0.h>
#include "text.h"
#include "assets/updater/tiles.h"

__attribute__((section(".iramx_screen.1")))
ws_screen_cell_t screen_1[32 * 18];
__attribute__((section(".iramx_tile")))
ws_tile_t tiles[256];

void text_clear(void) {
	memset(screen_1, 0, sizeof(screen_1));
}

void text_scroll_up_middle(bool lighten) {
	memmove(screen_1, screen_1 + 32, (32 * 9 - 4) * 2);
	memset(screen_1 + (32 * 9), 0, 32 * 2);
	if (lighten) ws_screen_modify_tiles(screen_1, ~SCR_ENTRY_PALETTE_MASK, SCR_ENTRY_PALETTE(1),
		0, 0, 28, 9);
}

void text_init(void) {
	bool color = ws_system_is_color();
	outportb(IO_SYSTEM_CTRL2, color ? 0x80 : 0x00);
	if (color) {
		for (int i = 0; i < 16; i++) {
			MEM_COLOR_PALETTE(i)[0] = 0x0FFF;
			MEM_COLOR_PALETTE(i)[1] = 0x0000;
		}
		MEM_COLOR_PALETTE(1)[1] = 0x0BBB;
	} else {
	    ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
		for (int i = 0; i < 16; i++) {
	    	outportw(IO_SCR_PAL(i), MONO_PAL_COLORS(0, 7, 2, 5));
		}
		outportw(IO_SCR_PAL(1), MONO_PAL_COLORS(0, 2, 2, 5));
	}
    outportb(IO_SCR_BASE, SCR1_BASE(screen_1));
	memset(tiles, 0, 0x200);
	wsx_zx0_decompress((uint16_t*) 0x2200, gfx_tiles);
	text_clear();
	outportw(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE);
}

void text_puts(void *_dest, uint16_t tile, uint16_t x, uint16_t y, const char __far* text) {
	if (x == TEXT_CENTERED) {
		x = (DISPLAY_WIDTH - strlen(text)) >> 1;
	}
    uint16_t *dest = (uint16_t*) _dest;
    dest += (y << 5); dest += x;
    while (*text != 0) {
        *(dest++) = ((uint8_t) *(text++)) | tile;
    }
}

void text_put_screen(void *_dest, uint16_t tile, uint16_t x, uint16_t y, uint16_t height, const char __far* __far* text) {
	for (int i = 0; i < height; i++) {
		if (text[i] != NULL) {
			text_puts(_dest, tile, x, y + i, text[i]);
		}
	}
}

__attribute__((format(printf, 5, 6)))
void text_printf(void *_dest, uint16_t tile, uint16_t x, uint16_t y, const char __far* format, ...) {
    uint16_t *dest = (uint16_t*) _dest;
    char buf[129];
    va_list val;
    va_start(val, format);
    vsnprintf(buf, sizeof(buf), format, val);
    va_end(val);
    text_puts(dest, tile, x, y, buf);
}
