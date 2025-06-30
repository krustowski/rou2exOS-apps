# rou2exOS apps

## C++

How to compile and link a C++ source (`print.cpp`) to a flat binary (`print.bin`):

```
gcc -Xlinker "-T./linker.ld" -nostdinc -nostdlib -ffreestanding -I include print.cpp
llvm-objcopy -O binary a.out print.bin 
```

`linker.ld`

```
/* Linker script for a basic C++ project memory layout */

ENTRY(main)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  .text :
  {
    /*KEEP(*(.isr_vector))*/
    *(.text*)
    *(.rodata*)
    _etext = .;
  } > FLASH

  .data : AT (ADDR(.text) + SIZEOF(.text))
  {
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM

  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM

  /* Stack and heap can be defined here if needed */
  _end = .;
}

```

## NASM

```
nasm -f bin -o print.bin print.asm
```
