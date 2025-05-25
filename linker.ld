ENTRY(_start)

SECTIONS
{
  . = 0x100000;

  .multiboot : {
    *(.multiboot)
  }

  .text : {
    *(.text)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data)
  }

  .bss : {
    *(.bss)
    *(COMMON)
  }
      . = 0x200000;
      .gen_taskpool : { *(.rodata.gen_taskpool_bin*) 
      				*(.data.gen_taskpool_bin*) }
}
