ENTRY(_main)

SECTIONS {
  /* Load code at this address in physical memory */
  . = 0x600000;

  .text : {
    KEEP(*(.text._main)) /* keep TinyGo entry point if named */
    *(.text*)             /* all .text */
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

