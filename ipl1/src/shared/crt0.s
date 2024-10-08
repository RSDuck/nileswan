/**
 * Copyright (c) 2022, 2023, 2024 Adrian "asie" Siekierka
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

	.arch	i186
	.code16
	.intel_syntax noprefix

	.section .header, "ax"
header:
	.word _start
	.word __sector_count
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0

	.section .start, "ax"
	.global _start

_start:
	// performed by IPL0
	// cli

	xor	ax, ax
	// CS = 0x0000
	mov	ds, ax
	mov	es, ax
	mov	ss, ax

	// configure SP
	mov	sp, 0x3200

	// clear int enable
	out	0xB2, al

	// clear BSS
	// assumption: AX = 0
	mov	di, offset "__sbss"
	mov	cx, offset "__lwbss"
	cld
	rep	stosw

	// configure default interrupt base
	mov	al, 0x08
	out	0xB0, al

	jmp	main
