#[unsafe(no_mangle)]
pub fn syscall(num: u64, arg1: u64, arg2: u64) -> u64 {
    let ret: u64;
    unsafe {
        core::arch::asm!(
            "mov rdi, {0:r}",
            "mov rsi, {1:r}",
            "mov rax, {2:r}",
            "int 0x7f",
            in(reg) arg1,
            in(reg) arg2,
            in(reg) num,
            lateout("rax") ret,
            options(nostack),
        );
    }
    ret
}

#[repr(C, packed)]
#[derive(Default)]
pub struct SysInfo {
    pub system_name: [u8; 32],
    pub system_user: [u8; 32],
    pub system_version: [u8; 8],
    pub system_uptime: u32,
}

pub fn exit(code: u64) -> ! {
    syscall(0x00, code, 0);

    loop {}
}

pub fn sysinfo() -> Option<SysInfo> {
    let mut sysinfo: SysInfo = Default::default();

    let ret = syscall(0x01, 0x01, &mut sysinfo as *mut SysInfo as u64);

    if ret == 0 {
        return Some(sysinfo);
    }
    None
}

pub fn print(input: &[u8]) -> u64 {
    syscall(0x10, input.as_ptr() as u64, input.len() as u64)
}
