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
		*(.rodata ".rodata.*")
		*(.data ".data.*")

		/* Put packed data last so it can be overwritten by stack later */
		*(.packed_data ".packed_data.*")

		. = ALIGN (2);
		__etext = .;
	}

	/* TODO: Place before .text so .packed_data works as intended */
	.bss . (NOLOAD) :
	{
		__sbss = .;
		*(.bss ".bss.*")
		. = ALIGN (2);
		__ebss = .;
	}
	__lbss = SIZEOF(.bss);
	__lwbss = (__lbss + 1) / 2;

	__sector_count = ((__etext - __stext) + 511) / 512;
}
