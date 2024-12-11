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

	.arch	i186
	.code16
	.intel_syntax noprefix

	.section .text, "ax"
    // ax = destination
    // dx = source
    // cx = count
    // stack = fill value
	.global memcpy8to16
memcpy8to16:
    push es
    push ds
    pop es
    push si
    push di
    push bp
    mov bp, sp

    mov di, ax
    mov si, dx
    mov ax, [bp + WF_PLATFORM_CALL_STACK_OFFSET(8)]
    
    cld
memcpy8to16_loop:
    lodsb
    stosw
    loop memcpy8to16_loop

    pop bp
    pop di
    pop si
    pop es
    ASM_PLATFORM_RET 0x2

    .global print_hex_number
print_hex_number:
    push di
    push es
    mov di, ax
    xor ax, ax
    mov es, ax
    mov ah, 0x01
    mov cx, 4

print_hex_number_loop:
    mov bx, dx
    shr bx, 12
    es mov al, [hexchars + bx]
    stosw
    shl dx, 4
    loop print_hex_number_loop

    mov ax, di
    pop es
    pop di
    ret
