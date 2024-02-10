OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS
{
	. = SEGMENT_START ("text-segment", 0x0060);

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
}
