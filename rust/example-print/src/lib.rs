#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[no_mangle]
pub extern "C" fn entry(arg: u32) -> ! {
    let s = *b"Hello, interrupt!\n";

    syscall(0x10, s.as_ptr() as u64, s.len() as u64);
    syscall(0x00, 0 as u64, 0 as u64);

    loop {}
}

#[unsafe(no_mangle)]
pub fn syscall(n: u64, a1: u64, a2: u64) -> () {
    let ret: u64;
    unsafe {
        core::arch::asm!(
            "mov rdi, {0}",
            "mov rsi, {1}",
            "mov rax, {2}",
            "int 0x7f",
            in(reg) a1,
            in(reg) a2,
            in(reg) n,
            options(nostack),
        );
    }
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    if let Some(_location) = info.location() {
        //string(vga_index, location.file().as_bytes(), Color::Red);
        //number(vga_index, location.line() as u64);
    } else {
        //
    }

    loop {}
}

