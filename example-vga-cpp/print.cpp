#define ADDR 0xb8000

long syscall(long num, long a1, long a2, long a3) {
    long ret;
    asm volatile (
        "int $0x7f"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

extern "C" int main()
{
    /*char *p = (char *)ADDR;
    *p = 'j';
    p = p + 2;
    *p = 'a';*/

    return syscall(1, 0, 0, 0);
}

