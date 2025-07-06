ENTRY(entry)

SECTIONS
{
  . = 0x600000;

  .text : {
    KEEP(*(.text.entry))     
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
  }
}

