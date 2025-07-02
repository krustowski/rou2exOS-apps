package main

import "unsafe"

//go:extern __bss_start
var bssStart uintptr

//go:extern __bss_end
var bssEnd uintptr

func zeroBSS() {
	ptr := bssStart
	for ptr < bssEnd {
		*(*uint8)(unsafe.Pointer(ptr)) = 0
		ptr++
	}
}

func main() {
	zeroBSS()

	vga := (*[2]uint16)(unsafe.Pointer(uintptr(0xb8000)))
	vga[0] = 0x2F4F // 'O'
	vga[1] = 0x2F59 // 'Y'
}
