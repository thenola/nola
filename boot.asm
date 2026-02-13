; boot.asm - Multiboot2 header and 32-bit entry point
; Этот файл является точкой входа для GRUB (multiboot2).
; Здесь определяется заголовок multiboot2 и стартовая 32-битная точка входа,
; которая настраивает стек и передаёт управление в код инициализации long mode.

BITS 32

SECTION .multiboot2_header
align 8

multiboot2_header_start:
    dd 0xE85250D6            ; magic
    dd 0                     ; architecture (0 = i386, GRUB загружает в 32-bit protected mode)
    dd multiboot2_header_end - multiboot2_header_start ; header_length
    dd 0x100000000 - (0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header_start))
                             ; checksum: (magic + arch + length + checksum) == 0 (mod 2^32)

; --- Tag: entry address (указывает на нашу 32-битную точку входа) ---
align 8
    dw 3                     ; type = 3 (entry address)
    dw 0                     ; flags = 0 (необязательный)
    dd 12                    ; size = 12 bytes (8 header + 4 entry_addr)
    dd start                 ; адрес точки входа (32-битный адрес)

; --- Tag: end (обязателен) ---
align 8
    dw 0                     ; type = 0 (end tag)
    dw 0                     ; flags
    dd 8                     ; size

multiboot2_header_end:

SECTION .text
align 16
global start                 ; точка входа для GRUB
extern long_mode_init        ; функция инициализации long mode (из long_mode_init.asm)
extern stack_top             ; вершина стека, определена в другом объекте

; GRUB передаёт:
;  EAX = magic (multiboot2)
;  EBX = адрес структуры multiboot2 information

start:
    cli                      ; запрещаем прерывания на время инициализации

    ; Настраиваем стек (32-битный)
    mov esp, stack_top

    ; Сохраняем multiboot параметры (если захотим использовать позже)
    push eax                 ; magic
    push ebx                 ; multiboot info pointer

    ; Переход в long mode и дальнейшая инициализация
    call long_mode_init

    ; Если по какой-то причине мы вернулись — зависаем
.halt:
    hlt
    jmp .halt

SECTION .bss
align 16
stack_bottom:
    resb 16384               ; 16 KiB стек
stack_top:

