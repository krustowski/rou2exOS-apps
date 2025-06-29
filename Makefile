build_floppy:
	@dd \
        if=/dev/zero \
        of=fat.img \
        bs=512 \
        count=2880
	@mkfs.fat \
        -F 12 \
        fat.img
	@echo "Hello from floppy!" > /tmp/hello.txt
	@mcopy -i fat.img /tmp/hello.txt ::HELLO.TXT 
	@mcopy -i fat.img ./example-vga-cpp/print.bin ::PRINT.BIN

run:
	@qemu-system-x86_64 -boot d -cdrom r2.iso -fda fat.img -serial stdio