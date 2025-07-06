#![no_main]
#![no_std]

mod abi;

#[unsafe(no_mangle)]
extern "C" fn entry() -> ! {
    abi::print(b"Starting shell...\n\n");

    abi::exit(42);
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
