export CGO_ENABLED=0

build:
	@docker run --rm -v $(shell pwd):/src tinygo/tinygo:0.38.0 tinygo build \
		-o /src/hello.elf \
		-scheduler=none \
		-no-debug \
		-opt=z \
		-gc=none \
		-ldflags "--extldflags='-T /src/linker.ld --no-pie --static --strip-debug'" \
		/src/hello.go
	@llvm-objcopy -O binary hello.elf hello.bin 
