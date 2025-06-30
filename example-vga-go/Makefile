build:
	@docker run --rm -v $(shell pwd):/src tinygo/tinygo:0.38.0 tinygo build \
		-o /src/hello.o \
		-scheduler=none \
		-no-debug \
		-gc=none \
		-ldflags "-extldflags=-T./linker.ld" \
		/src/hello.go
	@llvm-objcopy -O binary hello.o hello.bin 
