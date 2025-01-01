/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan IPL1 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan IPL1 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan IPL1. If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <ws.h>
#include <nile.h>
#include "ipc.h"

void ipc_init(void) {
	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	if (MEM_NILE_IPC->magic != NILE_IPC_MAGIC) {
		MEM_NILE_IPC->magic = NILE_IPC_MAGIC;
		memset(((uint8_t __far*) MEM_NILE_IPC) + 2, 0, sizeof(nile_ipc_t) - 2);

		MEM_NILE_IPC->boot_entrypoint = *((uint8_t*) 0x3800);
		memcpy(&(MEM_NILE_IPC->boot_regs), (void*) 0x3802, 184 + 24);
	}
}
