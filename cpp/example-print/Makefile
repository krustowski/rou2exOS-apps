build:
	@gcc -m64 -Xlinker "-Tlinker.ld" -nostdinc -nostdlib -ffreestanding -I ../include print.cpp
	@llvm-objcopy -O binary a.out print.bin 

dump:
	@llvm-objdump -d a.out
