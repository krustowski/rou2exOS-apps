ENTRY(main)

SECTIONS {
  /* Load code at this address in physical memory */
  . = 0x600000;

  .text : {
    KEEP(*(.text.main))
    *(.text*)             
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
    *(COMMON)
  }
}

