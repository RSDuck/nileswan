ROMSeg          equ 0xF000
IRAMSeg         equ 0x0000
SRAMSeg         equ 0x1000

IPL1FlashAddr   equ 0x20000

IPL1Target      equ 0x0400
IPL1Size        equ 0x0400

LCD_SEG_DATA    equ 0x15

DEBUG_LEDS      equ 0xDF

SPI_DATA        equ 0xE0
SPI_CNT         equ 0xE1

SPI_CNT_BUSY    equ 0x01
SPI_CNT_KEEP_CS equ 0x02

org 0xFFF00
section .text
text_start:

; does a final SPI transaction after which /CS
; returns to high
; al - in/out data
exchangeByteLast:
    xor al, al
    out SPI_CNT, al

; fall through into exchangeByte

; performs an one byte SPI transaction
; al - in/out data
exchangeByte:
    out SPI_DATA, al

.waitIdle:
    in al, SPI_CNT
    test al, SPI_CNT_BUSY
    jne .waitIdle
    in al, SPI_DATA

    ret

; initiate SPI flash read
; bx - address bits 8-23
; cl - address bits 0-7
startRead:
    mov al, SPI_CNT_KEEP_CS
    out SPI_CNT, al

    mov al, 0x03
    call exchangeByte
    mov al, bh
    call exchangeByte
    mov al, bl
    call exchangeByte
    mov al, cl
    call exchangeByte

    ret

start:
    mov ax, IRAMSeg
    mov ds, ax

    mov al, 1
    out LCD_SEG_DATA, al

    mov bx, IPL1FlashAddr>>8
    mov cl, IPL1FlashAddr&0xFF
    call startRead

    mov si, IPL1Target
    mov cx, IPL1Size
.copyLoop:
    call exchangeByte
    mov [si], al
    inc si
    dec cx
    jne .copyLoop

    call exchangeByteLast

    mov al, 2
    out LCD_SEG_DATA, al

    jmp IRAMSeg:IPL1Target

times	(256-16)-$+text_start db 0xFF

jmp ROMSeg:start

db	0x00
db	0x42	; Developer ID
db	0x01    ; Color
db	0x01	; Cart number
db	0x00    ; Version
db	0x00    ; ROM size
db	0x03    ; Save type
dw	0x0004  ; Flags
dw	0x0000	; Checksum
