#ifndef __VWF8_H__
#define __VWF8_H__

#include <stddef.h>
#include <wonderful.h>
#include <ws.h>

// 8-high variable width font system
int vwf8_get_string_width(const char __wf_rom* s);
int vwf8_draw_char(uint8_t __wf_iram* tile, uint8_t chr, int x);
int vwf8_draw_string(uint8_t __wf_iram* tile, const char __wf_rom* s, int x);

#endif
