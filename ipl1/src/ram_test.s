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

#define INDICATOR_ADDR 0x3C76

	.arch	i186
	.code16
	.intel_syntax noprefix

	.section .text, "ax"
    // ax - bank count

    // uses ax, cx
.macro xorshift_ax_cx
    // x ^= x << 7
    mov cx, ax
    shl cx, 7
    xor ax, cx

    // x ^= x >> 9
    mov cl, ah
    shr cl, 1
    xor al, cl

    // x ^= x << 8
    xor ah, al
.endm

    // ax = pointer to result structure (of size dx bytes)
    // dx = number of banks to test
	.global ram_fault_test
ram_fault_test:
    push ds
    push es
    push si
    push di
    mov bx, ax
    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    cld
    call ram_fault_test_perform

    pop di
    pop si
    pop es
    pop ds
    ASM_PLATFORM_RET

ram_fault_test_perform:
    push dx
    push dx

    xor di, di

    // initialize random value
    mov ax, 12345
ram_fault_test_write_outer_loop:
    // dx = dx - 1, bank = dx
    xchg ax, dx
    dec ax
    out IO_BANK_2003_RAM, ax
    xchg ax, dx
ram_fault_test_write_loop:
    // store random word to memory
    stosw
    xorshift_ax_cx
    // have we finished the page?
    test di, di
    jnz ram_fault_test_write_loop

    // increment indicator
    ss mov cx, word ptr [INDICATOR_ADDR]
    inc cx
    or cx, 0x140
    and cx, 0x17F
    ss mov word ptr [INDICATOR_ADDR], cx

    // have we finished all pages?
    test dx, dx
    jnz ram_fault_test_write_outer_loop

    // restore bank counter
    pop dx
    // initialize random value
    mov ax, 12345
ram_fault_test_read_outer_loop:
    // dx = dx - 1, bank = dx
    xchg ax, dx
    dec ax
    out IO_BANK_2003_RAM, ax
    xchg ax, dx
ram_fault_test_read_loop:
    // compare memory with random word
    scasw
    // is there a difference?
    jnz ram_fault_test_read_found
ram_fault_test_read_next:
    // advance PRNG
    xorshift_ax_cx
    // have we finished the page?
    test di, di
    jnz ram_fault_test_read_loop
ram_fault_test_read_page_done:
    // write "no error" result
    cmp word ptr ss:[bx], 0x0121
    je ram_fault_test_read_page_done_error
    mov word ptr ss:[bx], 0x012E
ram_fault_test_read_page_done_error:
    call ram_fault_test_incr_bx

    // increment indicator
    ss mov cx, word ptr [INDICATOR_ADDR]
    inc cx
    or cx, 0x140
    and cx, 0x17F
    ss mov word ptr [INDICATOR_ADDR], cx

    // have we finished all pages?
    test dx, dx
    jnz ram_fault_test_read_outer_loop
    // restore bank counter
    pop dx
    ret

ram_fault_test_read_found:
    // write "error" result
    mov word ptr ss:[bx], 0x0121
    // write "error" location
    pusha
    // dx = bank
    mov word ptr ss:[0x3C50], 0x013F
    mov ax, 0x3C40
    call print_hex_number
    // di = offset + 2
    mov dx, di
    dec dx
    dec dx
    call print_hex_number
    
    // wait for keypress
ram_fault_test_read_found_keypress:
    call ws_keypad_scan
    and ax, 0x0DDD
    jz ram_fault_test_read_found_keypress
    push ax
ram_fault_test_read_found_keypress2:
    call ws_keypad_scan
    and ax, 0x0DDD
    jnz ram_fault_test_read_found_keypress2
    pop ax
    mov word ptr ss:[0x3C50], 0x0120
    test ah, 0x0F
    jnz ram_fault_test_read_clear_bank_only
    popa
    // clear pointer, read next page
    xor di, di
    jmp ram_fault_test_read_page_done_error
ram_fault_test_read_clear_bank_only:
    popa
    jmp ram_fault_test_read_next

ram_fault_test_incr_bx:
    mov cx, bx
    add bx, 2
    xor cx, bx
    and cx, 0x20
    jz ram_fault_test_incr_bx_end
    add bx, 32
ram_fault_test_incr_bx_end:
    ret
