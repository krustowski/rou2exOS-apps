#include "syscall.h"

extern "C" int64_t syscall(int64_t number, int64_t arg1, int64_t arg2, int64_t arg3) 
{
    int64_t ret;
    asm volatile (
        "int $0x7f"
        : "=a"(ret)
        : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

extern char stack_top[];

extern "C" int64_t main()
{
    asm volatile ("mov %0, %%rsp" :: "r"(stack_top));

    int64_t len = 0;
    volatile const char *s = "Hello from C++!\n";

    while (s[len]) ++len;

    int64_t ret = syscall(ScSysinfo, (int64_t)s, len, 0);

    return ret;
}

