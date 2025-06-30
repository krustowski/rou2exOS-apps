#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[no_mangle]
#[unsafe(link_section = ".text.entry")]
pub extern "C" fn entry(arg: u32) -> u32 {
    /*let vga = 0xb8000 as *mut u16;
    unsafe {
        *vga.offset(0) = 0x1E41; // 'A'
        *vga.offset(1) = 0x1E30 + (arg as u16); // '0' + arg
    }*/

    return syscall(2, arg as u64, 34) as u32
}

#[inline(always)]
pub fn syscall(n: u64, a1: u64, a2: u64) -> u64 {
    let ret: u64;
    unsafe {
        core::arch::asm!(
            "int 0x7f",
            inlateout("rax") n => ret,
            in("rdi") a1,
            in("rsi") a2,
            options(nostack),
        );
    }
    ret
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    if let Some(location) = info.location() {
        //string(vga_index, location.file().as_bytes(), Color::Red);
        //number(vga_index, location.line() as u64);
    } else {
        //
    }

    loop {}
}

