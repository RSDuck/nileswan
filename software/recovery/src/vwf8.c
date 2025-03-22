#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "../build/assets/fonts.h"

#include "iram.h"

// #define VWF8_SUPPORT_4BPP

int vwf8_get_string_width(const char __wf_rom* s) {
    int w = 0;

    while (*s) {
        uint8_t chr = *(s++);
        const uint8_t __wf_rom* font = font8_bitmap + ((chr - 0x20) * 9);
        w += *font;
    }

    return w;
}

static const uint8_t __wf_rom font_width_mask[] = {
	0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF
};

int vwf8_draw_char(uint8_t __wf_iram* tile, uint8_t chr, int x) {
    const uint8_t __wf_rom* font = font8_bitmap + ((chr - 0x20) * 9);

    int width = *(font++);
    int next_x = x + width;
    int x_fine = (17 - width - (x & 0x7));

    uint16_t mask = ~(font_width_mask[width] << x_fine);

#ifdef VWF8_SUPPORT_4BPP
    if (ws_system_mode_get() & SYSTEM_CTRL2_4BPP) {
        tile += (x >> 3) << 5;

        for (int i = 0; i < 8; i++, font++, tile += 4) {
            uint16_t shift = (*font) << x_fine;
            tile[0] = ((tile[0] & (mask >> 8)) | (shift >> 8)) & 0xFF;
            tile[32] = ((tile[32] & mask) | shift) & 0xFF;
        }
    } else
#endif
    {
        tile += (x >> 3) << 4;

        for (int i = 0; i < 8; i++, font++, tile += 2) {
            uint16_t shift = (*font) << x_fine;
            tile[0] = ((tile[0] & (mask >> 8)) | (shift >> 8)) & 0xFF;
            tile[16] = ((tile[16] & mask) | shift) & 0xFF;
        }
    }

    return next_x;
}

int vwf8_draw_string(uint8_t __wf_iram* tile, const char __wf_rom* s, int x) {
	while (*s) {
		x = vwf8_draw_char(tile, *(s++), x);
	}
	return x;
}
