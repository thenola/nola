; long_mode_init.asm - проверка поддержки long mode, настройка paging и переход в 64-битный режим
;
; Этот файл содержит:
;  - Проверку CPUID на поддержку long mode
;  - Создание минимальной иерархии таблиц страниц (PML4, PDPT, PD, PT)
;  - Включение PAE и paging, установка бита LME в MSR EFER
;  - Загрузку GDT для 64-битного кода
;  - Дальний переход в 64-битный long mode и вызов kernel_main()

BITS 32

SECTION .text
global long_mode_init
extern gdt64_descriptor
extern stack_top
extern mb_magic
extern mb_info

; Простая иерархия таблиц страниц.
; Мы создаём:
;  - один PML4
;  - один PDPT
;  - один PD
;  - одну PT, которая отображает первые 2 MiB памяти тождественно (identity mapping).

SECTION .bss
align 4096
global pml4
pml4:
    resq 512

align 4096
global pdpt
pdpt:
    resq 512

align 4096
global pd
pd:
    resq 512

align 4096
global pt0
pt0:
    resq 512

SECTION .text

%define CR0_PE      0x00000001
%define CR0_PG      0x80000000

%define CR4_PAE     0x00000020
%define CR4_PGE     0x00000080

%define MSR_EFER    0xC0000080
%define EFER_LME    0x00000100

%define PAGE_PRESENT  0x1
%define PAGE_RW       0x2

;-----------------------------------------------------------------------------
; long_mode_init
; Вход: стек содержит [magic, multiboot_info_ptr] (но здесь не используется).
;-----------------------------------------------------------------------------

long_mode_init:
    ; Проверяем поддержку long mode через CPUID.
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode         ; если расширенный leaf 0x80000001 недоступен

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29        ; бит 29 EDX - Long Mode
    jz .no_long_mode

    ; Подготавливаем таблицы страниц.
    call setup_paging

    ; Включаем PAE, PGE в CR4.
    mov eax, cr4
    or eax, CR4_PAE | CR4_PGE
    mov cr4, eax

    ; Загружаем CR3 адресом PML4.
    mov eax, pml4
    mov cr3, eax

    ; Включаем long mode через MSR EFER.
    mov ecx, MSR_EFER
    rdmsr                    ; EDX:EAX = EFER
    or eax, EFER_LME
    wrmsr

    ; Включаем paging (и PE, если вдруг выключен).
    mov eax, cr0
    or eax, CR0_PE | CR0_PG
    mov cr0, eax

    ; Загружаем GDT с 64-битными сегментами.
    lgdt [gdt64_descriptor]

    ; Дальний прыжок для перехода в 64-битный режим.
    ; Селектор кода = 0x08 (первый ненулевой дескриптор в GDT).
    jmp 0x08:long_mode_entry

.no_long_mode:
    ; Если long mode не поддерживается, просто зависаем.
.halt:
    cli
    hlt
    jmp .halt

;-----------------------------------------------------------------------------
; setup_paging
; Создаёт простую иерархию страниц:
;  PML4[0] -> PDPT
;  PDPT[0] -> PD
;  PD[0]   -> PT0
;  PT0[i]  -> i * 4096, первые 2 MiB.
;-----------------------------------------------------------------------------

setup_paging:
    ; Обнуляем таблицы (PML4, PDPT, PD, PT0).

    push edi
    push eax
    push ecx

    ; zero pml4
    mov edi, pml4
    mov ecx, 512*8/4          ; 512 qwords -> 1024 dword
    xor eax, eax
    rep stosd

    ; zero pdpt
    mov edi, pdpt
    mov ecx, 512*8/4
    xor eax, eax
    rep stosd

    ; zero pd
    mov edi, pd
    mov ecx, 512*8/4
    xor eax, eax
    rep stosd

    ; zero pt0
    mov edi, pt0
    mov ecx, 512*8/4
    xor eax, eax
    rep stosd

    ; Связи между уровнями:
    ; PML4[0] = phys(pdpt) | P | RW
    mov eax, pdpt
    or eax, PAGE_PRESENT | PAGE_RW
    mov [pml4 + 0*8], eax
    mov dword [pml4 + 0*8 + 4], 0

    ; PDPT[0] = phys(pd) | P | RW
    mov eax, pd
    or eax, PAGE_PRESENT | PAGE_RW
    mov [pdpt + 0*8], eax
    mov dword [pdpt + 0*8 + 4], 0

    ; PD[0] = phys(pt0) | P | RW
    mov eax, pt0
    or eax, PAGE_PRESENT | PAGE_RW
    mov [pd + 0*8], eax
    mov dword [pd + 0*8 + 4], 0

    ; PT0: identity mapping первых 2 MiB памяти.
    mov ecx, 512              ; 512 страниц по 4 KiB = 2 MiB
    mov edi, pt0
    xor eax, eax              ; физический адрес, начинаем с 0
.map_loop:
    ; запись низких 32 бит адреса + флаги
    mov edx, eax
    or edx, PAGE_PRESENT | PAGE_RW
    mov [edi], edx
    ; старшие 32 бита адреса = 0
    mov dword [edi + 4], 0

    add eax, 4096             ; следующая страница
    add edi, 8                ; следующий PTE

    dec ecx
    jnz .map_loop

    pop ecx
    pop eax
    pop edi
    ret

;-----------------------------------------------------------------------------
; 64-битный вход.
;-----------------------------------------------------------------------------

BITS 64

extern kernel_main
extern mb_magic
extern mb_info

long_mode_entry:
    ; Настраиваем 64-битный стек (используем тот же стек, но в RSP).
    mov rsp, stack_top

    ; Обнулим сегментные регистры данных (они игнорируются в long mode,
    ; но это хорошая практика).
    mov ax, 0x10              ; селектор 64-битного data сегмента
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Здесь мы уже в long mode, можно вызывать 64-битный C-код.
    ; Подготовим аргументы для kernel_main(magic, info_ptr)
    ; в соответствии с System V ABI: RDI, RSI.

    mov eax, dword [rel mb_magic]
    mov edi, eax                     ; 1‑й аргумент: magic (32 бита)

    mov eax, dword [rel mb_info]
    mov rsi, rax                     ; 2‑й аргумент: info ptr (zero‑extend до 64 бит)

    call kernel_main

.halt64:
    cli
    hlt
    jmp .halt64

