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

#ifndef __TEXT_H__
#define __TEXT_H__

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include <ws.h>

#define TEXT_CENTERED 0xFFFF

extern ws_screen_cell_t screen_1[32 * 18];
extern ws_tile_t tiles[256];

void text_clear(void);
void text_scroll_up_middle(bool lighten);
void text_init(void);
void text_puts(void *_dest, uint16_t tile, uint16_t x, uint16_t y, const char __far* text);
void text_put_screen(void *_dest, uint16_t tile, uint16_t x, uint16_t y, uint16_t height, const char __far* __far* text);
__attribute__((format(printf, 5, 6)))
void text_printf(void *_dest, uint16_t tile, uint16_t x, uint16_t y, const char __far* format, ...);

#endif /* __TEXT_H__ */
