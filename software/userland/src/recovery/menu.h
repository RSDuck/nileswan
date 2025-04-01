#ifndef __MENU_H__
#define __MENU_H__

#include <stddef.h>
#include <wonderful.h>
#include <ws.h>
#include "console.h"

#define MENU_MAX_ENTRY_COUNT CONSOLE_LINE_COUNT

int menu_run(const char __far* __far* options);

#endif
