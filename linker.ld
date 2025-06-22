ENTRY(_start)

	SECTIONS { 
		. = 0x100000;

		.multiboot : {
			KEEP(*(.multiboot))
		}

		.text : {
			*(.text)
		}

		.rodata : {
			*(.rodata*)
		}
		.data : {
			*(.data*)
		}
		.bss : {
			*(.bss)
			*(COMMON)
		}
		.stack : {
			. = ALIGN(16);
			_stack_bottom = .;
			. += 4000;
			_stack_top = .;
	}
}
