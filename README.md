# rou2exOS apps

An application suite for the [rou2exOS kernel](https://github.com/krustowski/rou2exOS). All aplications must follow the syscall ABI specification. 

Languages currently implementing (parts) of ABI:

+ C++
+ Go (using TinyGo, WIP)
+ NASM 
+ Rust

Each listed language has its workspace in a related directory in the repository root. Each implementation example should include:

+ linker script (`linker.ld`)
+ Makefile
+ README.md (optional)
+ the source code file(s)

The linker script should ensure proper memory alignment with the `.text` section at the beginning.

## Executables

At the moment, all executables (elf-x86-64) have to be converted into a flat memory binary file. ELF files cannot be loaded directly yet. See tooling below for more. 

To run a binary in the `r2` shell, it has to be loaded onto a floppy image with FAT12 filesystem (see the root Makefile). When it's loaded, boot the system using attached ISO image and the floppy image:

```
qemu-system-x86_64 -boot d -m 2G -cdrom r2.iso -fda fat.img
```

In the shell, ensure the file is there and then just load it and run (do not include the binary extension):

```
dir 
run PRINT
```

## Development Tools

Tooling strictly depends on the selected language. However, these tools are required to be able to compile, link, convert, and debug the source code:

+ mtools
+ llvm tools 
+ lld
+ qemu
+ gnumake
+ gcc (cross compiler)
+ nasm 

