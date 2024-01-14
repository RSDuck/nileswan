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
__attribute__((section(".iramx_2bpp_2000")))
IRAM_EXTERN ws_tile_t tile_2bpp_bank0[512];
__attribute__((section(".iramx_4bpp_4000")))
IRAM_EXTERN ws_tile_4bpp_t tile_4bpp_bank0[512];
__attribute__((section(".iramx_4bpp_8000")))
IRAM_EXTERN ws_tile_4bpp_t tile_4bpp_bank1[512];

// Declare two distinct screens.
// Using ".iram" instead of ".iramx" will cause them to be automatically
// cleared on startup.
__attribute__((section(".iram_screen.a")))
IRAM_EXTERN ws_screen_cell_t screen_1[32*32];
__attribute__((section(".iram_screen.b")))
IRAM_EXTERN ws_screen_cell_t screen_2[32*32];

// Declare a sprite table.
// __attribute__((section(".iramx_sprite")))
// IRAM_EXTERN ws_sprite_t sprites[128];

// Declare a wavetable.
// __attribute__((section(".iramx_wave")))
// IRAM_EXTERN ws_sound_wavetable_t wave_ram;

/* IRAM LAYOUT DECLARATION END */

#undef IRAM_EXTERN

#endif /* __IRAM_H__ */
