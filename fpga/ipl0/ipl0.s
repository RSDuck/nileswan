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

IPL0Size	 equ 512

IPL1FlashAddr    equ 0x20000
IPL1AltFlashAddr equ 0x24000
IPL1SelftestFlashAddr equ 0x28000
IPL1IRAMAddr	 equ 0x0060

; assumption: at minimum 0x0600
IPL1IRAMSize	 equ 0x3E00

NileIPLConsoleType	equ 0x59
NileIPLConsoleTypeWS	equ 0
NileIPLConsoleTypePCv2	equ 1

	org 0x0000
; 4000:0000 - boot ROM alternate boot location
	jmp 0xf000:start_pcv2
	db 0, 0, 0, 'nileswan'
; 4000:0010 - boot ROM alternate boot location (PCv2)
; F000:0010 - IPL0 start location
start_pcv2:
; Initialize hardware
	cli

; Do some tricks to store the full register state on boot
; in memory area 0x0040 - 0x0058

; Store correct state of AX and DS in a screen LUT
; We need to initialize DS to access RAM, but we want to preserve it
        out     0x20, ax
        mov     ax, ds
        out     0x22, ax

	mov	al, NileIPLConsoleTypePCv2
	mov	[NileIPLConsoleType], al
	jmp	start_shared
start:
; Initialize hardware
	cli

; Do some tricks to store the full register state on boot
; in memory area 0x0040 - 0x0058

; Store correct state of AX and DS in a screen LUT
; We need to initialize DS to access RAM, but we want to preserve it
        out     0x20, ax
        mov     ax, ds
        out     0x22, ax

	mov	al, NileIPLConsoleTypeWS
	mov	[NileIPLConsoleType], al
start_shared:
        ; Initialize DS to 0x0000
        mov     ax, 0x0000
        mov     ds, ax

        ; Preserve registers in RAM
        in      ax, 0x20
        mov     [0x40], ax ; AX
        mov     [0x42], bx
        mov     [0x44], cx
        mov     [0x46], dx
        mov     [0x48], sp
        mov     [0x4A], bp
        mov     [0x4C], si
        mov     [0x4E], di
        in      ax, 0x22
        mov     [0x50], ax ; DS
        mov     [0x52], es
        mov     [0x54], ss

        ; Init SS/SP here so we can PUSHF
        mov     ax, ds ; assumption: DS == 0
        mov     ss, ax
	mov	si, ax ; also, zero SI for a later assumption

        pushf
        pop	di
        mov     [0x56], di

; Prepare flash command write: read from address 0x03 onwards
	mov di, ax ; assumption: AX == 0
	mov ax, SRAMSeg
	mov es, ax
	mov ax, NILE_BANK_RAM_TX
	out RAM_BANK_2003, ax
	mov ax, NILE_BANK_ROM_RX
	out ROM_BANK_0_2003, ax

; Vary IPL1 load location depending on a special keybind.
	call keypadScan
	mov bx, ax
	and ax, (KEY_Y1 | KEY_B)
	cmp ax, (KEY_Y1 | KEY_B)
	je altFlashAddr
	mov ax, bx
	and ax, (KEY_X3 | KEY_B)
	cmp ax, (KEY_X3 | KEY_B)
	je selftestFlashAddr
	mov bx, IPL1FlashAddr >> 8
	jmp postFlashAddr
selftestFlashAddr:
	mov bx, IPL1SelftestFlashAddr >> 8
	jmp postFlashAddr
altFlashAddr:
	mov bx, IPL1AltFlashAddr >> 8
postFlashAddr:

; Write 0x03, BH, BL, 0x00 to SPI TX buffer
	mov ax, bx
	mov al, 0x03
	stosw
	mov ax, bx
	mov ah, 0x00
	stosw

; Initialize SPI write to flash device, flip buffer
	mov ax, ((4 - 1) | SPI_MODE_WRITE | SPI_CNT_CS | SPI_CNT_BUFFER | SPI_CNT_BUSY)
	out NILE_SPI_CNT, ax
	call spiSpinwait

; Initialize first SPI read from flash device, flip buffer
	mov ax, ((512 - 1) | SPI_MODE_READ | SPI_CNT_CS | SPI_CNT_BUSY)
	out NILE_SPI_CNT, ax

; Set up read loop, wait for SPI read to finish
	mov cx, (IPL1IRAMSize >> 9) - 2
	mov di, IPL1IRAMAddr
	mov es, si ; assumption: SI == 0
	mov ax, 0x2000
	mov ds, ax
	call spiSpinwait

readLoop:
	in ax, NILE_SPI_CNT
; Initialize SPI read from flash device, flip buffer
	and ax, SPI_CNT_BUFFER
	xor ax, ((512 - 1) | SPI_MODE_READ | SPI_CNT_CS | SPI_CNT_BUFFER | SPI_CNT_BUSY)
	out NILE_SPI_CNT, ax

; Read 512 bytes from flipped buffer
	mov dx, cx
	mov cx, 0x100
	rep movsw
	mov cx, dx

; Wait for SPI read to finish, read next 512 bytes
	call spiSpinwait
	loop readLoop

readComplete:
; De-initialize SPI device, flip buffer
	in ax, NILE_SPI_CNT
	and ax, SPI_CNT_BUFFER
	xor ax, SPI_CNT_BUFFER
	out NILE_SPI_CNT, ax

; Read final 512 bytes from flipped buffer
	mov cx, 0x100
	rep movsw

; Jump to IPL1
	jmp 0x0000:IPL1IRAMAddr

; === Utility functions ===

; Wait until SPI is no longer busy. Destroys AL.
spiSpinwait:
	in al, NILE_SPI_CNT+1
	test al, 0x80
	jnz spiSpinwait
	ret

; Scan keypad. Return result in AX
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

	pop	dx
	pop	cx
	ret

	times (IPL0Size-16)-($-$$) db 0xFF

; 0xFFFF:0x0000 - boot ROM primary boot location + header
	jmp 0xf000:start

	db	0x00	; Maintenance
	db	0x42	; Developer ID
	db	0x01    ; Color
	db	0x81	; Cart number + Disable IEEPROM write protect
	db	0x00    ; Version
	db	0x00    ; ROM size
	db	0x05	; Save type
	dw	0x0004  ; Flags
	dw	0x0000	; Checksum
