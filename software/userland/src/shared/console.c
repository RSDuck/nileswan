#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/system.h>
#include "console.h"
#include "config.h"
#include "iram.h"
#include "vwf8.h"

static uint8_t __wf_rom tile_border[16] = {
	0x00, 0x00,
	0x00, 0xFF,
	0xFF, 0xFF,
	0x00, 0xFF,
	0xFF, 0xFF,
	0x00, 0xFF,
	0xFF, 0xFF,
	0xFF, 0x00
};
static const char __wf_rom s_ok[] = "[OK]";
static const char __wf_rom s_fail[] = "[FAIL]";

#define TILE_LINE(y) MEM_TILE((y)*28)
#define HEADER_TILE_LINE TILE_LINE(16)

static inline void console_ui_init(void) {
	outportw(IO_DISPLAY_CTRL, 0);

	// Configure palettes.
	ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
	// Palette 0 - black text (1) on white background (0) with grey highlights (2)
	outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 5, 2));
	// Palette 1 - white text (1) on dark gray background (0) with black highlights (2)
	outportw(IO_SCR_PAL_1, MONO_PAL_COLORS(4, 0, 6, 2));
	// Palette 2 - white text (1) on black background (0) with grey highlights (2)
	outportw(IO_SCR_PAL_2, MONO_PAL_COLORS(7, 0, 2, 5));
    
    // Enable color mode.
    if (ws_system_is_color()) {
        ws_system_mode_set(WS_MODE_COLOR);
        MEM_COLOR_PALETTE(0)[0] = 0xFFF;
        MEM_COLOR_PALETTE(0)[1] = 0x000;
        MEM_COLOR_PALETTE(0)[2] = 0x444;
        MEM_COLOR_PALETTE(0)[3] = 0xBBB;
        MEM_COLOR_PALETTE(1)[0] = 0x777;
        MEM_COLOR_PALETTE(1)[1] = 0xFFF;
        MEM_COLOR_PALETTE(1)[2] = 0x222;
        MEM_COLOR_PALETTE(1)[3] = 0xBBB;
        MEM_COLOR_PALETTE(2)[0] = 0x000;
        MEM_COLOR_PALETTE(2)[1] = 0xFFF;
        MEM_COLOR_PALETTE(2)[2] = 0xBBB;
        MEM_COLOR_PALETTE(2)[3] = 0x444;
    }

	// Tile 511 - border tile
	memcpy(MEM_TILE(511), tile_border, sizeof(tile_border));

	// Initialize screen layouts.
	for (int iy = 0; iy < 16; iy++) {
		int io = iy*28;
		int so = iy*32;
		for (int ix = 0; ix < 28; ix++) {
			screen_1[so+ix] = io+ix;
			screen_1[so+ix+512] = io+ix;
		}
	}
	for (int i = 0; i < 28; i++) {
		screen_2[i] = (28*16)+i+SCR_ENTRY_PALETTE(1);
		screen_2[i+32] = 511+SCR_ENTRY_PALETTE(1);
	}

	// Configure screens.
	outportb(IO_SCR_BASE, SCR1_BASE(screen_1) | SCR2_BASE(screen_2));
	outportw(IO_SCR1_SCRL_X, (240 << 8));
	outportw(IO_SCR2_SCRL_X, (240 << 8));
	outportw(IO_SCR2_WIN_X1, 0);
	outportw(IO_SCR2_WIN_X2, (15 << 8) | 239);
	outportw(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE | DISPLAY_SCR2_ENABLE | DISPLAY_SCR2_WIN_INSIDE);
}

void console_init(void) {
    console_ui_init();
#ifdef CONFIG_CONSOLE_SERIAL
    ws_serial_open(SERIAL_BAUD_38400);
#endif
    console_clear();
}

#ifdef CONFIG_CONSOLE_SERIAL
static void serial_puts(const char __far* str) {
    while (*str) {
        ws_serial_putc(*str);
        str++;
    }
}
#else
#define serial_puts(...) {}
#endif

void console_draw_header(const char __far* str) {
    int width = vwf8_get_string_width(str);
    int x = (DISPLAY_WIDTH_PX - width) >> 1;
    memset(HEADER_TILE_LINE, 0, 28 * 16);
    vwf8_draw_string(HEADER_TILE_LINE, str, x);
}

void console_print_header(const char __far* str) {
    console_draw_header(str);

#ifdef CONFIG_CONSOLE_SERIAL
    // Print to serial
    ws_serial_putc('=');
    ws_serial_putc('=');
    ws_serial_putc(' ');
    serial_puts(str);
    ws_serial_putc(' ');
    ws_serial_putc('=');
    ws_serial_putc('=');
    ws_serial_putc('\r');
    ws_serial_putc('\n');
#endif
}

uint8_t console_x;
uint8_t console_y;

static void console_set_y(uint8_t y) {
    console_x = 1;
    y &= 0xF;
    console_y = y;
    outportb(IO_SCR1_SCRL_Y, 120 + (y << 3));
}

void console_clear(void) {
    memset(TILE_LINE(0), 0, 28*16*16);
    console_set_y(15);
}

static void console_draw_newline(void) {
    memset(TILE_LINE((console_y + 1) & 0xF), 0, 28*16);
    console_set_y(console_y + 1);
}

void console_print_newline(void) { 
    console_draw_newline();

#ifdef CONFIG_CONSOLE_SERIAL
    ws_serial_putc('\r');
    ws_serial_putc('\n');
#endif
}

#define CONSOLE_STACK_BUFFER_LENGTH 81

int console_draw(int x, int y, uint16_t flags, const char __far* str) {
    uint8_t *tile = TILE_LINE(y);
    if (flags & CONSOLE_FLAG_HIGHLIGHT) {
        tile++;
    }
    if (flags & (CONSOLE_FLAG_CENTER | CONSOLE_FLAG_RIGHT)) {
        int width = vwf8_get_string_width(str);
        if (flags & CONSOLE_FLAG_CENTER) {
            x = (DISPLAY_WIDTH_PX - width) >> 1;
        } else if (flags & CONSOLE_FLAG_RIGHT) {
            x = (DISPLAY_WIDTH_PX - x - width);
        }
    }
    return vwf8_draw_string(tile, str, x);
}

int console_vdrawf(int x, int y, uint16_t flags, const char __far* format, va_list val) {
    char buf[CONSOLE_STACK_BUFFER_LENGTH];
    vsnprintf(buf, CONSOLE_STACK_BUFFER_LENGTH, format, val);
    return console_draw(x, y, flags, buf);
}

int console_drawf(int x, int y, uint16_t flags, const char __far* format, ...) {
    char buf[CONSOLE_STACK_BUFFER_LENGTH];
    va_list val;
    va_start(val, format);
    vsnprintf(buf, CONSOLE_STACK_BUFFER_LENGTH, format, val);
    va_end(val);
    return console_draw(x, y, flags, buf);
}

void console_print(uint16_t flags, const char __far* str) {
new_line:
    ;
    int x = console_x;
    uint8_t *tile = TILE_LINE(console_y);
    if (flags & CONSOLE_FLAG_HIGHLIGHT) {
        tile++;
    }
    if (flags & (CONSOLE_FLAG_CENTER | CONSOLE_FLAG_RIGHT)) {
        int width = vwf8_get_string_width(str);
        if (flags & CONSOLE_FLAG_CENTER) {
            x = (DISPLAY_WIDTH_PX - width) >> 1;
        } else if (flags & CONSOLE_FLAG_RIGHT) {
#ifdef CONFIG_CONSOLE_SERIAL
            if (!(flags & CONSOLE_FLAG_NO_SERIAL)) {
                ws_serial_putc(' ');
            }
#endif
            x = (DISPLAY_WIDTH_PX - width);
        }
    }

    while (*str) {
        if (*str == '\n') {
#ifdef CONFIG_CONSOLE_SERIAL
            if (flags & CONSOLE_FLAG_NO_SERIAL) {
                console_draw_newline();
            } else {
                console_print_newline();
            }
#else
            console_draw_newline();
#endif
            str++;
            flags &= ~(CONSOLE_FLAG_RIGHT);
            goto new_line;
        } else if (*str == '\t') {
            x += 10;
        } else {
            int width = (flags & CONSOLE_FLAG_MONOSPACE) ? 5 : vwf8_get_char_width(*str);
            if ((x + width) > DISPLAY_WIDTH_PX) {
                console_draw_newline();
                flags &= ~(CONSOLE_FLAG_RIGHT);
                goto new_line;
            }
            if (flags & CONSOLE_FLAG_MONOSPACE) {
                vwf8_draw_char(tile, *str, x);
                x += 5;
            } else {
    		    x = vwf8_draw_char(tile, *str, x);
            }
        }
#ifdef CONFIG_CONSOLE_SERIAL
        if (!(flags & CONSOLE_FLAG_NO_SERIAL)) {
            ws_serial_putc(*str);
        }
#endif
        str++;
	}
    if (!(flags & (CONSOLE_FLAG_CENTER | CONSOLE_FLAG_RIGHT))) {
	    console_x = x;
    }
}

void console_putc(uint16_t flags, uint16_t ch) {
    ch &= 0xFF;
    console_print(flags, (const char __far*) &ch);
}

void console_vprintf(uint16_t flags, const char __far* format, va_list val) {
    char buf[CONSOLE_STACK_BUFFER_LENGTH];
    vsnprintf(buf, CONSOLE_STACK_BUFFER_LENGTH, format, val);
    return console_print(flags, buf);
}

void console_printf(uint16_t flags, const char __far* format, ...) {
    char buf[CONSOLE_STACK_BUFFER_LENGTH];
    va_list val;
    va_start(val, format);
    vsnprintf(buf, CONSOLE_STACK_BUFFER_LENGTH, format, val);
    va_end(val);
    return console_print(flags, buf);
}

bool console_print_status(bool status) {
    console_print(CONSOLE_FLAG_RIGHT | CONSOLE_FLAG_HIGHLIGHT | (status ? CONSOLE_FLAG_NO_SERIAL : 0), status ? s_ok : s_fail);
    return status;
}
