	bits 16
	cpu 186

	org 0xFFFC
; BootFriend header
	db 'b', 'F'
	dw 0xFFFF

start:
	cli
	cld

; Enable IPL lockout
	mov al, 0x87
	out 0xA0, al
; Clear interrupts
	mov al, 0x00
	out 0xB2, al
	dec al
	out 0xB6, al

; SS:SP=0000:4000
; DS:SI=CS:[ipl1 start]
; ES:DI=0000:[ipl1 start]
	xor ax, ax
	mov es, ax
	mov ss, ax
	mov sp, 0x4000
	mov si, ipl1_data
	cs mov di, [si]
	push cs
	pop ds
	mov si, (ipl1_data + 16)
	mov cx, (ipl1_data_end - ipl1_data - 16)

	push es
	push di

	rep movsb

; Clear IPL0 register storage
	xor ax, ax
	mov di, 0x3800
	mov cx, 24
	rep stosb

; Set initial SP
	es mov word [0x3800 + 10], 0x2000
	es mov word [0x3800 + 24], 0xF002

; Clear registers
	mov bx, ax
	mov cx, ax
	mov dx, ax
	mov bp, ax
	mov si, ax
	mov di, ax
	mov ds, ax
; ES and SS are already cleared

	retf

ipl1_data:
%ifdef safe
	incbin "safe.bin"
%else
%ifdef factory
	incbin "factory.bin"
%else
	incbin "boot.bin"
%endif
%endif

ipl1_data_end:
