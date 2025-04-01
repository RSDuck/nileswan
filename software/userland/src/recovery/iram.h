/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * 240p-test-ws is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * 240p-test-ws is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with 240p-test-ws. If not, see <https://www.gnu.org/licenses/>.
 */

// Internal RAM layout declaration header.
// See https://wonderful.asie.pl/doc/wswan/guide/topics/memory-management/

#ifndef __IRAM_H__
#define __IRAM_H__

#include <wonderful.h>
#include <ws.h>

// Remember to define IRAM_IMPLEMENTATION in only one .c file!
// Defining it in zero .c files will cause missing symbols.
// Defining it in multiple .c files will cause duplicate symbols.

#ifdef IRAM_IMPLEMENTATION
#define IRAM_EXTERN
#else
#define IRAM_EXTERN extern
#endif

/* IRAM LAYOUT DECLARATION START */
// Everything up until "IRAM LAYOUT DECLARATION END" is user-configurable.

// Reserve all tile memory in both 2BPP and 4BPP modes.
__attribute__((section(".iram_2bpp_2000")))
IRAM_EXTERN ws_tile_t tile_2bpp_bank0[512];
__attribute__((section(".iramx_4bpp_4000")))
IRAM_EXTERN ws_tile_4bpp_t tile_4bpp_bank0[512];
__attribute__((section(".iramx_4bpp_8000")))
IRAM_EXTERN ws_tile_4bpp_t tile_4bpp_bank1[512];

// Declare two distinct screens.
__attribute__((section(".iram_1780")))
IRAM_EXTERN uint16_t __wf_iram screen_2[32*2];
__attribute__((section(".iram_1800")))
IRAM_EXTERN uint16_t __wf_iram screen_1[32*32];

/* IRAM LAYOUT DECLARATION END */

#undef IRAM_EXTERN

#endif /* __IRAM_H__ */
