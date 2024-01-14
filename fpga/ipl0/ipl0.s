%include "../common/swan.inc"

IRAMStubTarget  equ 0x0500
IPL1FlashAddr   equ 0x20000

IPL1NumSegments equ (128/64)

org ((ROMSeg0<<4)|0xFF00)
section .text
text_start:

; to be copied into IRAM
iramStub:
    xor al, al
    out BLKMEM_CTRL, al
    mov al, IPL1NumSegments-1
    out PSRAM_BANK_MASK, al
    mov al, 0x1
    out LCD_SEG_DATA, al
    jmp ROMSeg0:0xFFF0
iramStubEnd:

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
    cli

    mov al, 1
    out LCD_SEG_DATA, al

    mov bx, IPL1FlashAddr>>8
    mov cl, IPL1FlashAddr&0xFF
    call startRead

    mov ax, SRAMSeg
    mov ds, ax

    mov al, MEMORY_ENABLE_SELF_FLASH
    out MEMORY_CTRL, al

    xor di, di
    xor cx, cx
.segmentLoop:
    mov al, cl
    out RAM_BANK, al

.wordLoop:
    call exchangeByte
    mov ah, al
    call exchangeByte
    out PSRAM_UPPER_BITS, al

    mov [di], ah
    inc di
    mov [di], ah
    inc di
    jne .wordLoop

    inc cx
    cmp cx, IPL1NumSegments
    jl .segmentLoop

    call exchangeByteLast

; Check memory just copied
;    mov bx, IPL1FlashAddr>>8
;    mov cl, IPL1FlashAddr&0xFF
;    call startRead
;
;    mov ax, ROMSeg1
;    mov ds, ax
;
;    xor di, di
;    xor cx, cx
;.segmentLoop2:
;    mov al, cl
;    out ROM_BANK_1, al
;
;.wordLoop2:
;    call exchangeByte
;    mov ah, al
;    call exchangeByte
;    xchg al, ah
;
;    cmp ax, [di]
;    jne .fail
;
;    add di, 2
;    jne .wordLoop2
;
;    inc cx
;    cmp cx, IPL1NumSegments
;    jl .segmentLoop2
;
;.correct:
;    mov al, 0x18
;    out LCD_SEG_DATA, al
;    jmp .correct
;
;.fail:
;    mov al, 0x3F
;    out LCD_SEG_DATA, al
;    jmp .fail
;

    mov ax, ROMSeg0
    mov ds, ax
    mov ax, IRAMSeg
    mov es, ax
    mov cx, iramStubEnd-iramStub
    mov di, IRAMStubTarget
    mov si, iramStub
    cld
    rep movsb

    xor al, al
    out MEMORY_ENABLE_SELF_FLASH, al

    mov al, 2
    out LCD_SEG_DATA, al

    jmp IRAMSeg:IRAMStubTarget

wsheader text_start, ROMSeg0, start, 0x100, SAVETYPE_SRAM_128KB