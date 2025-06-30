package main

import "unsafe"

func main() {
	vga := (*[2]uint16)(unsafe.Pointer(uintptr(0xb8000)))
	vga[0] = 0x2F4F // 'O'
	vga[1] = 0x2F59 // 'Y'
}
