; Copyright (c) 2023, 2024 Adrian "asie" Siekierka
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.

	bits 16
	cpu 186

	incbin "ipl1.bin"

start:
	cli

	xor ax, ax
	mov es, ax
	mov si, ax

	mov bx, 0xe000
	mov ds, bx

	mov di, 0x0060 ; Start offset
	mov bx, di
	mov cx, 0x4000
	sub cx, di
	shr cx, 1
	cld
	rep movsw

	push 0x0000
	push bx
	retf

times 0x1FFF0-($-$$) db 0xFF ; Padding

; WonderSwan ROM header

	jmp 0xe000:start
	db 0 ; Maintenance
	db 0 ; Publisher ID
	db 1 ; Color
	db 0 ; Game ID
	db 0x80 ; Game version
	db 0 ; ROM size
	db 5 ; RAM size
	db 4 ; Flags
	db 1 ; Mapper
	dw 0xFFFF ; Checksum
