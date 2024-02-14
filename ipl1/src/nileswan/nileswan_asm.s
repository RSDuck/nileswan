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
    ss mov dx, [nile_spi_timeout_ms] // timeout loop multiplier
nile_spi_wait_busy_loop_start:
    mov cx, 240
    // ~1ms timeout loop
    .balign 2, 0x90
nile_spi_wait_busy_loop:
    in al, (IO_NILE_SPI_CNT + 1) // 6 cycles
    and al, 0x80 // 1 cycle
    jz nile_spi_wait_busy_done
    loop nile_spi_wait_busy_loop // 5 cycles
    dec dx
    jnz nile_spi_wait_busy_loop_start

    // timeout, AL = 0x80
    mov ax, 0
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
    

    .section .fartext.s.nile_spi_rx, "ax"
    .align 2
    .global nile_spi_rx
    // AX length, DX mode
nile_spi_rx:
    dec ax
    or ax, dx
    // AX config options
nile_spi_rx_inner:
    push ax

    // if (!nile_spi_wait_busy()) return false;
    ASM_PLATFORM_CALL nile_spi_wait_busy
    pop dx
    test al, al
    jz nile_spi_rx_done

    // uint16_t cnt = inportw(IO_NILE_SPI_CNT);
    in ax, IO_NILE_SPI_CNT
    // uint16_t new_cnt = ((size - 1) | mode) | (cnt & 0x7800);
    and ax, 0x7800
    or ax, dx
    or ah, (NILE_SPI_START >> 8)
    // outportw(IO_NILE_SPI_CNT, new_cnt | NILE_SPI_START);
    out IO_NILE_SPI_CNT, ax
    
    mov ax, 1
nile_spi_rx_done:
    ASM_PLATFORM_RET
    

    .section .fartext.s.nile_spi_rx_flip, "ax"
    .align 2
    .global nile_spi_rx_flip
    // AX length, DX mode
nile_spi_rx_flip:
    ASM_PLATFORM_CALL nile_spi_rx
    test al, al
    jz nile_spi_rx_flip_done
    ASM_PLATFORM_CALL nile_spi_wait_busy
    test al, al
    jz nile_spi_rx_flip_done
    in al, (IO_NILE_SPI_CNT+1)
    xor al, (NILE_SPI_BUFFER_IDX >> 8)
    out (IO_NILE_SPI_CNT+1), al

nile_spi_rx_flip_done:
    ASM_PLATFORM_RET

    

    .section .fartext.s.nile_spi_rx_copy, "ax"
    .align 2
    .global nile_spi_rx_copy
    // DX:AX pointer
    // CX - length
    // stack - mode
nile_spi_rx_copy:
    mov bx, sp
    push ds
    push si
    push es
    push di
    push dx
    push ax

#ifdef __IA16_CMODEL_IS_FAR_TEXT
    ss mov ax, [bx + 4]
#else
    ss mov ax, [bx + 2]
#endif

    mov bx, cx
    dec cx
    or ax, cx
    ASM_PLATFORM_CALL nile_spi_rx_inner
    test al, al
    jz nile_spi_rx_copy_done1

    // if (!nile_spi_wait_busy()) return false;
    ASM_PLATFORM_CALL nile_spi_wait_busy
    test al, al
    jz nile_spi_rx_copy_done1

    mov cx, bx
#ifndef NILESWAN_IPL1
    // volatile uint16_t prev_bank = inportw(IO_BANK_2003_ROM1);
    in ax, IO_BANK_2003_ROM1
    xchg ax, bx
#endif
    // outportw(IO_NILE_SPI_CNT, new_cnt ^ NILE_SPI_BUFFER_IDX);
    in al, (IO_NILE_SPI_CNT+1)
    xor al, (NILE_SPI_BUFFER_IDX >> 8)
    out (IO_NILE_SPI_CNT+1), al
    // outportw(IO_BANK_2003_ROM1, NILE_SEG_ROM_RX);
    mov ax, NILE_SEG_ROM_RX
    out IO_BANK_2003_ROM1, ax

    // memcpy(buf, MK_FP(0x3000, 0x0000), size);
    pop di
    pop es
    push 0x3000
    pop ds
    xor si, si
    cld
    shr cx, 1
    rep movsw
    jnc nile_spi_rx_copy_no_byte
    movsb
nile_spi_rx_copy_no_byte:

#ifndef NILESWAN_IPL1
    // outportw(IO_BANK_2003_ROM1, prev_bank);
    xchg ax, bx
    out IO_BANK_2003_ROM1, ax
#endif
    mov ax, 1

nile_spi_rx_copy_done:
    pop di
    pop es
    pop si
    pop ds
    ASM_PLATFORM_RET 0x2

nile_spi_rx_copy_done1:
    pop ax
    pop dx
    jmp nile_spi_rx_copy_done
