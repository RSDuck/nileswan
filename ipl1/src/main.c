// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Adrian "asie" Siekierka, 2023
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/hardware.h>
#include <ws/system.h>

// Assets
#include "mono/pyramid.h"
#include "mono/swan.h"

#define IRAM_IMPLEMENTATION
#include "iram.h"

void main(void) {
	// Disable display.
	outportb(IO_LCD_SEG, LCD_SEG_AUX1|LCD_SEG_AUX3);
	outportw(IO_DISPLAY_CTRL, 0);

	// Set the screen address.
	outportb(IO_SCR_BASE, SCR1_BASE(screen_1) | SCR2_BASE(screen_2));

	// Clear scroll registers.
	outportb(IO_SCR1_SCRL_X, 0);
	outportb(IO_SCR1_SCRL_Y, 0);
	outportb(IO_SCR2_SCRL_X, 0);
	outportb(IO_SCR2_SCRL_Y, 0);

	outportb(IO_LCD_SEG, LCD_SEG_AUX1|LCD_SEG_AUX2);

	// Clear tile 0.
	memset(MEM_TILE(0), 0, sizeof(ws_tile_t));

	// Clear screen 2.
	// On a "mono" model, only palettes 4-7 and 12-15 are translucent.
	ws_screen_fill_tiles(screen_2, SCR_ENTRY_PALETTE(12), 0, 0, 32, 32);

	outportb(IO_LCD_SEG, LCD_SEG_AUX1);

	// Copy tiles.
	memcpy(MEM_TILE(1), gfx_mono_pyramid_tiles, gfx_mono_pyramid_tiles_size);
	memcpy(MEM_TILE(384), gfx_mono_swan_tiles, gfx_mono_swan_tiles_size);

	outportb(IO_LCD_SEG, LCD_SEG_AUX2);

	// Configure mono palettes.
	ws_portcpy(IO_SCR_PAL_0, gfx_mono_pyramid_palette, gfx_mono_pyramid_palette_size);
	ws_portcpy(IO_SCR_PAL_12, gfx_mono_swan_palette, gfx_mono_swan_palette_size);

	outportb(IO_LCD_SEG, LCD_SEG_ORIENT_H);

	// Configure mono shade LUT.
	ws_display_set_shade_lut(SHADE_LUT_DEFAULT);

	outportb(IO_LCD_SEG, LCD_SEG_ORIENT_H|LCD_SEG_AUX1);

	// Write tilemaps.
	ws_screen_put_tiles(screen_1, gfx_mono_pyramid_map, 0, 0, 28, 18);
	ws_screen_put_tiles(screen_2, gfx_mono_swan_map, 28 - 16 - 2, 3, 16, 15);

	// Enable display: screen 1 and screen 2.
	outportw(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE | DISPLAY_SCR2_ENABLE);

	// Enable the VBlank interrupt.
	// Since this is the only interrupt enabled, "cpu_halt();" will only
	// wait for vertical blank.
	ws_hwint_set_default_handler_vblank();
	ws_hwint_enable(HWINT_VBLANK);

	// Enable CPU interrupts.
	cpu_irq_enable();

	// Wait indefinitely.
	while(1) {
		cpu_halt();

		outportb(IO_LCD_SEG, LCD_SEG_AUX2);

		// Move the second screen (with the swan) to the left by 1 pixel.
		outportb(IO_SCR2_SCRL_X, inportb(IO_SCR2_SCRL_X) + 1);
	}
}
