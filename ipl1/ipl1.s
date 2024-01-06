%include "../common/swan.inc"

%ifdef EMU

NUM_PAGES       equ 8

org 0x000
section .text

times	(64*1024)*7 db 0xFF

%else

NUM_PAGES       equ 64

org 0x400
section .text

%endif
text_start:

jmp start

; ax - state in/out
; bx - temp
xorshiftByte:
    mov bx, ax
    shl bx, 7
    xor ax, bx

    mov bx, ax
    shr bx, 9
    xor ax, bx

    mov bx, ax
    shl bx, 8
    xor ax, bx

    ret

start:
    cli

    mov ax, SRAMSeg
    mov ds, ax

    ; enable self flashing (aka PSRAM as SRAM address)
    ;mov al, 1
    ;out MEMORY_CTRL, al

    mov al, 0x6
    out LCD_SEG_DATA, al

    ; write page
    ; initialise xor shift
    xor cx, cx
    mov ax, 8921

.pageWriteLoop:
    xchg cl, al
    out RAM_BANK, al
    xchg cl, al

    xor si, si
.byteWriteLoop:
    call xorshiftByte
    mov [si], al

    inc si
    jne .byteWriteLoop

    inc cx
    cmp cx, NUM_PAGES
    jl .pageWriteLoop

    ; check page
    xor cx, cx
    mov ax, 8921

.pageCheckLoop:
    xchg cl, al
    out RAM_BANK, al
    xchg cl, al

    xor si, si
.byteCheckLoop:
    call xorshiftByte
    mov bl, [si]
    cmp al, bl
    jne .fail

    inc si
    jne .byteCheckLoop

    inc cx
    cmp cx, NUM_PAGES
    jl .pageCheckLoop

    mov al, 0x38
    out LCD_SEG_DATA, al

.forever:
    jmp .forever

.fail:
    mov al, 0x30
    out LCD_SEG_DATA, al

    jmp .forever

%ifdef EMU

wsheader text_start, ROMSeg0, start, 0x10000, SAVETYPE_SRAM_512KB

%else

times	256-$+text_start db 0xFF

%endif
