#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdarg.h>
#include <stddef.h>
#include <wonderful.h>
#include <ws.h>

#define CONSOLE_LINE_COUNT 16

#define CONSOLE_FLAG_CENTER (1 << 0)
#define CONSOLE_FLAG_RIGHT (1 << 1)
#define CONSOLE_FLAG_HIGHLIGHT (1 << 2)
#define CONSOLE_FLAG_MONOSPACE (1 << 3)
#define CONSOLE_FLAG_NO_SERIAL (1 << 4)

void console_init(void);
void console_draw_header(const char __far* str);
void console_print_header(const char __far* str);
void console_clear(void);
void console_print_newline(void);
int console_draw(int x, int y, uint16_t flags, const char __far* str);
int console_vdrawf(int x, int y, uint16_t flags, const char __far* format, va_list val);
int console_drawf(int x, int y, uint16_t flags, const char __far* format, ...);
void console_print(uint16_t flags, const char __far* str);
void console_vprintf(uint16_t flags, const char __far* format, va_list val);
void console_printf(uint16_t flags, const char __far* format, ...);
bool console_print_status(bool status);

#endif
