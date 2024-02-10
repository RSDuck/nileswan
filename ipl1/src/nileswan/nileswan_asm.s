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

#include <wonderful.h>
#include <ws.h>
#include "nileswan.h"

	.arch	i186
	.code16
	.intel_syntax noprefix

	.section .fartext.s.nile_spi_wait_busy, "ax"
    .align 2
    .global nile_spi_wait_busy
nile_spi_wait_busy:
    mov dx, 3 // timeout loop multiplier
nile_spi_wait_busy_loop_start:
    xor cx, cx
    dec cx
    // ~277ms timeout loop
    .balign 2, 0x90
nile_spi_wait_busy_loop:
    in al, (IO_NILE_SPI_CNT + 1) // 6 cycles
    and al, 0x80 // 1 cycle
    jz nile_spi_wait_busy_done
    loop nile_spi_wait_busy_loop // 5 cycles
    dec dx
    jnz nile_spi_wait_busy_loop_start

    // timeout, AL = 0x80
    mov al, 0
    ASM_PLATFORM_RET

nile_spi_wait_busy_done:
    // success, AL = 0x00
    inc ax
    ASM_PLATFORM_RET
