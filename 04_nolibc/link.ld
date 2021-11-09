SECTIONS
{
	_stext = .;
	.text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
	. = ALIGN(CONSTANT(MAXPAGESIZE));
	_etext = .;
	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
	. = ALIGN(CONSTANT(MAXPAGESIZE));
	_erodata = .;
	PROVIDE(_data = .);
	.data : { *(.dtata .datat.* .gnu.linkonce.d*) }
	_edata = .;
	.bss (NOLOAD) : {
		. = ALIGN(16);
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		__bss_end = .;
		__bss_size = (__bss_end - __bss_start)>>3;
	}
	_end = .;

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
