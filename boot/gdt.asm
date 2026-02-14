; gdt.asm - GDT для 64-битного режима с User segments и TSS
;
; Селекторы:
;   0x00 - null
;   0x08 - kernel code (DPL 0)
;   0x10 - kernel data (DPL 0)
;   0x18 - user code (DPL 3)
;   0x20 - user data (DPL 3)
;   0x28 - TSS

BITS 32

SECTION .data
align 8

global gdt64_descriptor
global tss64
global tss_descriptor

; TSS должна быть определена до GDT (дескриптор ссылается на tss64)
align 16
tss64:
    resd 1                      ; +0x00 reserved
    resq 1                      ; +0x04 RSP0 (kernel stack)
    resq 1                      ; +0x0C RSP1
    resq 1                      ; +0x14 RSP2
    resq 1                      ; +0x1C reserved
    resq 7                      ; +0x24 IST1..IST7
    resq 1                      ; +0x5C reserved
    resw 1                      ; +0x64 IOPB offset
    resb 2                      ; +0x66 reserved

align 8
gdt64:
    dq 0x0000000000000000       ; 0x00 null
    ; kernel code: type=0x9A, flags=0x20 (L)
    dq 0x00209A0000000000       ; 0x08
    ; kernel data: type=0x92
    dq 0x0000920000000000       ; 0x10
    ; user code: type=0xFA (DPL=3)
    dq 0x0020FA0000000000       ; 0x18
    ; user data: type=0xF2 (DPL=3)
    dq 0x0000F20000000000       ; 0x20
    ; TSS descriptor (16 bytes) — база заполняется в gdt_init()
tss_descriptor:
    dw 103                      ; limit 15:0
    dw 0                        ; base 15:0 (заполнить в C)
    db 0                        ; base 23:16
    db 0x89                     ; type: 64-bit TSS available
    db 0x00                     ; flags
    db 0                        ; base 31:24
    dd 0                        ; base 63:32
    dd 0                        ; reserved

gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64 - 1
    dd gdt64
