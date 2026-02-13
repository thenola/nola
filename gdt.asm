; gdt.asm - GDT для 64-битного режима + задел под TSS
;
; В этом файле определена таблица глобальных дескрипторов для long mode:
;  - null-дескриптор
;  - 64-битный code-сегмент
;  - 64-битный data-сегмент
; Также определён пустой 64-битный TSS и его дескриптор (пока не используется).

BITS 32

SECTION .data
align 8

global gdt64_descriptor

gdt64:
    dq 0x0000000000000000       ; null
    ; 64-bit code segment descriptor:
    ; base = 0, limit = 0, type = 0x9A, flags = 0x20 (L-bit)
    dq 0x00209A0000000000
    ; 64-bit data segment descriptor:
    ; base = 0, limit = 0, type = 0x92
    dq 0x0000920000000000

gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64 - 1
    dd gdt64

; -------------------- 64-bit TSS (пока не используется) --------------------

align 16

global tss64

tss64:
    ; 64-bit TSS имеет размер 104 байта.
    times 104 db 0

; Дескриптор TSS (16 байт) можно будет добавить в GDT позже при необходимости.

