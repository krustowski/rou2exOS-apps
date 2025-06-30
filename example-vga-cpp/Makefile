build:
	@gcc -Xlinker "-Tlinker.ld" -nostdinc -nostdlib -ffreestanding -I include print.cpp
	@llvm-objcopy -O binary a.out print.bin 
