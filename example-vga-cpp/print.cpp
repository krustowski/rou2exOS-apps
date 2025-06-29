#define ADDR 0xb8000

extern "C" int main()
{
    char *p = (char *)ADDR;
    *p = 'j';
    p = p + 2;
    *p = 'a';

    return 33;
}

