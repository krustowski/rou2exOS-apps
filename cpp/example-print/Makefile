build:
	@gcc -m64 -static -n -Xlinker "-Tlinker.ld" -nostdinc -nostdlib -ffreestanding -I ../include -o print.elf print.cpp
	@llvm-objcopy -O binary print.elf print.bin 

dump:
	@llvm-objdump -d a.out
