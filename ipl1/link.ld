OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS
{
	. = SEGMENT_START ("text-segment", 0x0040 - 16);

	.header : {
		KEEP(*(".header"))
		. = ALIGN (16);
	}

	.text : {
		__stext = .;
		KEEP(*(".start"))
		*(.text ".text.*")
		*(".fartext.*")
		*(".farrodata.*")
		__etext = .;
	}

	.data : {
		. = ALIGN (2);
		__sdata = .;
		*(.rodata ".rodata.*")
		*(.data ".data.*")
		. = ALIGN (2);
		__edata = .;
	}
	__ldata = SIZEOF(.data);
	__lwdata = (__ldata + 1) / 2;

	.bss . (NOLOAD) :
	{
		*(.bss ".bss.*")
		. = ALIGN (2);
		__ebss = .;
	}
	__lbss = SIZEOF(.bss);
	__lwbss = (__lbss + 1) / 2;

	__sector_count = ((__edata - __stext) + 511) / 512;
}
