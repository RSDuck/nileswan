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


    .section .fartext.s.nile_spi_tx, "ax"
    .align 2
    .global nile_spi_tx
    // DX:AX buffer, CX length
nile_spi_tx:
    // memcpy() preparation
    push si
    push di
    mov si, ax
    xor di, di

    // uint8_t prev_flash = inportb(IO_CART_FLASH);
    in al, IO_CART_FLASH
    push ax
    // uint8_t prev_bank = inportw(IO_BANK_2003_RAM);
    in ax, IO_BANK_2003_RAM
    push ax

    // outportb(IO_CART_FLASH, 0);
    xor ax, ax
    out IO_CART_FLASH, al
    // outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_TX);
    mov al, NILE_SEG_RAM_TX
    out IO_BANK_2003_RAM, ax

    // memcpy(MK_FP(0x1000, 0x0000), buf, size);
    dec cx
    push cx
    push es
    push ds
    mov ds, dx
    push 0x1000
    pop es
    cld
    shr cx, 1
    inc cx
    rep movsw
    pop ds
    pop es

    // if (!nile_spi_wait_busy()) return false;
    ASM_PLATFORM_CALL nile_spi_wait_busy
    pop cx // cx = size - 1
    test al, al
    jz nile_spi_tx_done

    // uint16_t cnt = inportw(IO_NILE_SPI_CNT);
    in ax, IO_NILE_SPI_CNT
    // uint16_t new_cnt = (size - 1) | NILE_SPI_MODE_WRITE | (cnt & 0x7800);
    // outportw(IO_NILE_SPI_CNT, (new_cnt ^ (NILE_SPI_BUFFER_IDX | NILE_SPI_START)));
    and ax, 0x7800
    xor ah, ((NILE_SPI_MODE_WRITE | NILE_SPI_BUFFER_IDX | NILE_SPI_START) >> 8)
    or ax, cx
    out IO_NILE_SPI_CNT, ax
    
    mov ax, 1
nile_spi_tx_done:
    xchg ax, dx
    // outportw(IO_BANK_2003_RAM, prev_bank);
    pop ax
    out IO_BANK_2003_RAM, ax
    // outportb(IO_CART_FLASH, prev_flash);
    pop ax
    out IO_CART_FLASH, al
    xchg ax, dx
    pop di
    pop si
    ASM_PLATFORM_RET
    
