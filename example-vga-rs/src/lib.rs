#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[no_mangle]
#[unsafe(link_section = ".text.entry")]
pub extern "C" fn entry(arg: u32) -> usize {
    let vga = 0xb8000 as *mut u16;
    unsafe {
        *vga.offset(0) = 0x1E41; // 'A'
        *vga.offset(1) = 0x1E30 + (arg as u16); // '0' + arg
    }

    return 999;
}

#[panic_handler]
fn lpanic(info: &PanicInfo) -> ! {
    if let Some(location) = info.location() {
        //string(vga_index, location.file().as_bytes(), Color::Red);
        //number(vga_index, location.line() as u64);
    } else {
        //
    }

    loop {}
}

