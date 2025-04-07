#include <wonderful.h>
#include <ws.h>

    .arch   i186
    .code16
    .intel_syntax noprefix

    .section .fartext.s.fetch_rtc_time, "ax"
    .global fetch_rtc_time
fetch_rtc_time:
    // AL = command to run
    // returns DX:AX = time
    push ax
    in al, 0xca
    test al, 0x10
    jnz error_still_active
    pop ax
    out 0xca, al

    // DX:CX = time
    xor dx, dx
    xor cx, cx

    // wait for bytes
1:
    in al, 0xca
    test al, 0x90
    jz 9f
    test al, 0x80
    jz 1b
    // read byte
    in al, 0xcb
    mov dl, ch
    mov ch, cl
    mov cl, al
    jmp 1b
9:
    mov ax, cx
    WF_PLATFORM_RET

error_still_active:
    mov dx, 0xFFFF
    mov dx, 0xFC00
    WF_PLATFORM_RET
