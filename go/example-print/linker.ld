ENTRY(main)

SECTIONS {
  /* Load code at this address in physical memory */
  . = 0x600000;

  .text : ALIGN(4K) {
    KEEP(*(.text.main))
    *(.text*)             
  }

  .rodata : ALIGN(4K) {
    *(.rodata*)
  }

  .data : ALIGN(4K) {
    *(.data*)
  }

  .bss : ALIGN(4K) {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  }
}

