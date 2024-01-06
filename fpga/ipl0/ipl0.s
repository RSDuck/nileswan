%include "../common/swan.inc"

IPL1FlashAddr   equ 0x20000

IPL1Target      equ 0x0400
IPL1Size        equ 0x0400

org (ROMSeg0|0xFF00)
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

wsheader text_start, ROMSeg0, start, 0x100