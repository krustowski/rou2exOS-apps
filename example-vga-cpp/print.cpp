#define ADDR 0xb8000

typedef long int64_t;

extern "C" __attribute__((naked)) int64_t main()
{
    int64_t num = 1, len = 18, ret;
    const char *s = "Hello from C++!\n\0";

    asm volatile (
        "int $0x7f"
        : "=a"(ret)
        : "a"(num), "D"((int64_t)s), "S"(len), "d"(0)
        : "rcx", "r11", "memory"
    );
    return ret;
}

