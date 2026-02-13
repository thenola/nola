/* linker.ld - линкер-скрипт для 64-битного ELF-ядра с multiboot2 */

ENTRY(start)

SECTIONS
{
    /* Загружаем ядро по адресу 1 MiB. */
    . = 1M;

    .multiboot2_header :
    {
        KEEP(*(.multiboot2_header))
    }

    .text ALIGN(0x1000) :
    {
        *(.text*)
    }

    .rodata ALIGN(0x1000) :
    {
        *(.rodata*)
    }

    .data ALIGN(0x1000) :
    {
        *(.data*)
    }

    .bss ALIGN(0x1000) :
    {
        *(COMMON)
        *(.bss*)
    }

    /* Символ конца бинарника для аллокатора страниц. */
    end = .;
}

