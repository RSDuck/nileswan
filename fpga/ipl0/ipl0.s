; Copyright (c) 2024 Adrian Siekierka
;
; Nileswan IPL0 is free software: you can redistribute it and/or modify it under
; the terms of the GNU General Public License as published by the Free
; Software Foundation, either version 3 of the License, or (at your option)
; any later version.
;
; Nileswan IPL0 is distributed in the hope that it will be useful, but WITHOUT
; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
; FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
; more details.
;
; You should have received a copy of the GNU General Public License along
; with Nileswan IPL0. If not, see <https://www.gnu.org/licenses/>.

%include "../common/swan.inc"

    bits 16
    cpu 186

    ; This allows us to access IRAM as 0x0000~0x3FFF on the CS segment.
NILE_IPL0_SEG            equ 0xf400
NILE_IPL0_TMP_RAM        equ 0xf800 ; 0000:3800
NILE_IPL0_STACK          equ 0x0000 ; 0000:4000
NILE_IPL0_SIZE           equ 512
NILE_FLASH_ADDR_IPL1     equ 0x28000
NILE_FLASH_ADDR_IPL1_ALT equ 0x2c000

    ; == Initialization / Boot state preservation ==

    ; 4000:0000 - boot ROM alternate boot location
    org 0x0000
    jmp 0xf400:start_entrypoint1
start_entrypoint1:
    cs mov byte [NILE_IPL0_TMP_RAM], 1
    jmp	start_shared
    times (16)-($-$$) db 0xFF

    ; 4000:0010 - boot ROM alternate boot location (PCv2)
    jmp 0xf400:start_entrypoint2
start_entrypoint2:
    cs mov byte [NILE_IPL0_TMP_RAM], 2
    jmp	start_shared

start:
    cs mov byte [NILE_IPL0_TMP_RAM], 0

start_shared:
    cs mov [NILE_IPL0_TMP_RAM + 18], ds
    cs mov [NILE_IPL0_TMP_RAM + 2], ax

    ; Initialize DS == CS
    mov ax, NILE_IPL0_SEG 
    mov ds, ax
    mov [NILE_IPL0_TMP_RAM + 4], bx
    mov [NILE_IPL0_TMP_RAM + 6], cx
    mov [NILE_IPL0_TMP_RAM + 8], dx
    mov [NILE_IPL0_TMP_RAM + 10], sp
    mov [NILE_IPL0_TMP_RAM + 12], bp
    mov [NILE_IPL0_TMP_RAM + 14], si
    mov [NILE_IPL0_TMP_RAM + 16], di
    mov [NILE_IPL0_TMP_RAM + 20], es
    mov [NILE_IPL0_TMP_RAM + 22], ss

    ; Initialize SS/SP
    mov ss, ax
    xor sp, sp

    ; Copy FLAGS
    pushf
    pop	di
    mov [NILE_IPL0_TMP_RAM + 24], di

    ; Clear interrupts
    cli

    ; Copy I/O port data
    push cs
    pop es
    xor dx, dx
    mov di, NILE_IPL0_TMP_RAM + 26
    mov cx, (0xC0 >> 1)
copyIoPortDataLoop:
    insw
    inc dx
    inc dx
    loop copyIoPortDataLoop

    ; == IPL1 loader ==

    ; Vary IPL1 load location depending on a special keybind.
    call keypadScan
    and ax, (KEY_X3 | KEY_B)
    cmp ax, (KEY_X3 | KEY_B)
    mov bx, NILE_FLASH_ADDR_IPL1 >> 8
    jne postFlashAddr
altFlashAddr:
    mov bx, NILE_FLASH_ADDR_IPL1_ALT >> 8
postFlashAddr:
    call spiStartRead

    ; == IPL1 loader / Read loop ==

    ; Initialize first SPI read (header) from flash device, flip buffer
    mov ax, ((16 - 1) | SPI_MODE_READ | SPI_CNT_DEV_FLASH | SPI_CNT_BUSY)
    out NILE_SPI_CNT, ax

    ; DS = 0x2000, ES = 0x0000 (, CS/SS = NILE_IPL0_SEG)
    mov ax, ROMSeg0
    mov ds, ax
    xor ax, ax
    mov es, ax

    ; Wait for SPI read to finish
    call spiSpinwait

    ; Initialize second SPI read (data) from flash device, flip buffer
    in ax, NILE_SPI_CNT
    and ax, SPI_CNT_BUFFER
    xor ax, ((512 - 1) | SPI_MODE_READ | SPI_CNT_DEV_FLASH | SPI_CNT_BUFFER | SPI_CNT_BUSY)
    out NILE_SPI_CNT, ax

    ; DI = Start address (push)
    mov di, [0x0000]
    push 0x0000
    push di

    ; CX = Sector count
    mov cx, [0x0002]

readLoop:
    call spiSpinwait

    ; Initialize SPI read from flash device, flip buffer
    in ax, NILE_SPI_CNT
    and ax, SPI_CNT_BUFFER
    xor ax, ((512 - 1) | SPI_MODE_READ | SPI_CNT_DEV_FLASH | SPI_CNT_BUFFER | SPI_CNT_BUSY)
    out NILE_SPI_CNT, ax

    ; Read 512 bytes from flipped buffer
    push cx
    mov cx, 0x100
    rep movsw
    pop cx

    ; Read next 512 bytes
    loop readLoop

readComplete:
    ; De-initialize SPI device
    xor ax, ax
    out NILE_SPI_CNT, ax

    ; Jump to IPL1
    retf

    ; === Utility functions ===

    ; BX = address
spiStartRead:
    ; Prepare flash command write: read from address 0x03 onwards
    xor si, si
    mov di, si
    push SRAMSeg
    pop es
    mov ax, NILE_BANK_RAM_TX
    out RAM_BANK_2003, ax
    mov ax, NILE_BANK_ROM_RX
    out ROM_BANK_0_2003, ax

; Write 0x03, BH, BL, 0x00 to SPI TX buffer
    mov ax, bx
    mov al, 0x03
    stosw
    mov ax, bx
    mov ah, 0x00
    stosw

; Initialize SPI write to flash device, flip buffer
    mov ax, ((4 - 1) | SPI_MODE_WRITE | SPI_CNT_DEV_FLASH | SPI_CNT_BUFFER | SPI_CNT_BUSY)
    out NILE_SPI_CNT, ax
    ; jmp spiSpinwait ; fallthrough

    ; Wait until SPI is no longer busy.
    ; Clobber: AL
spiSpinwait:
    in al, NILE_SPI_CNT+1
    test al, 0x80
    jnz spiSpinwait
    ret

    ; Scan keypad.
    ; Output: AX = keypad data
keypadScan:
    push	cx
    push	dx

    mov     dx, 0x00B5

    mov     al, 0x10
    out     dx, al
    daa
    in      al, dx
    and     al, 0x0F
    mov     ch, al

    mov     al, 0x20
    out     dx, al
    daa
    in      al, dx
    shl     al, 4
    mov     cl, al

    mov     al, 0x40
    out     dx, al
    daa
    in      al, dx
    and     al, 0x0F
    or      cl, al

    mov     ax, cx

    pop     dx
    pop     cx
    ret

    times (NILE_IPL0_SIZE-16)-($-$$) db 0xFF

; 0xFFFF:0x0000 - boot ROM primary boot location + header
    jmp NILE_IPL0_SEG:start

    db	0x00	; Maintenance
    db	0x42	; Developer ID
    db	0x01    ; Color
    db	0x81	; Cart number + Disable IEEPROM write protect
    db	0x00    ; Version
    db	0x00    ; ROM size
    db	0x05	; Save type
    dw	0x0004  ; Flags
    dw	0x0000	; Checksum
