ENTRY(entry)

SECTIONS
{
  . = 0x000000;

  .text : {
    KEEP(*(.text.startup))     /* explicitly keep entry function */
    *(.text*)                /* rest of text */
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
  }
}

