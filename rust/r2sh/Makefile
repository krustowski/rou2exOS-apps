build:
	@cargo rustc \
		--release \
		-- --emit=obj
	@ld.lld \
		-T ./linker.ld \
		-o ./app.elf \
		$(shell ls -t ./target/release/deps/r2sh-*.o | head -1)
	@llvm-objcopy \
		-O binary \
		app.elf app.bin 
