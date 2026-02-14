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
multiboot_entry_tag_start:
    dw 3                     ; type = 3 (entry address)
    dw 0                     ; flags = 0 (необязательный)
    dd multiboot_entry_tag_end - multiboot_entry_tag_start
    dd start                 ; адрес точки входа (32-битный адрес)
multiboot_entry_tag_end:

; --- Tag: framebuffer (1920x1080x32) ---
align 8
multiboot_fb_tag_start:
    dw 5
    dw 0
    dd multiboot_fb_tag_end - multiboot_fb_tag_start
    dd 1920
    dd 1080
    dd 32
multiboot_fb_tag_end:

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

    ; Сохраняем multiboot параметры в глобальные переменные
    mov [mb_magic], eax      ; magic
    mov [mb_info],  ebx      ; multiboot info pointer

    ; Переход в long mode и дальнейшая инициализация
    call long_mode_init

    ; Если по какой-то причине мы вернулись — зависаем
.halt:
    hlt
    jmp .halt

SECTION .bss
align 16

global mb_magic
global mb_info

mb_magic:
    resd 1
mb_info:
    resd 1

stack_bottom:
    resb 16384               ; 16 KiB стек
stack_top:

