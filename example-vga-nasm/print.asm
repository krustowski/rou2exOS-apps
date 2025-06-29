; org 0x600000
bits 64
_start:
    mov dword [0xB8000], 0x2F592F4F  ; "OY"
    cli
    mov rax, 42
    ret

