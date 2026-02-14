#include <stdint.h>
#include <stddef.h>

#include "vga.h"
#include "idt.h"
#include "gdt.h"
#include "process.h"
#include "paging.h"
#include "heap.h"
#include "multiboot2.h"
#include "shell.h"
#include "fs.h"
#include "config.h"

void kernel_main(uint32_t mb_magic, uint64_t mb_info_addr) {
    (void)mb_magic;

    /* Сохраняем multiboot info для парсинга (в т.ч. framebuffer 1920x1080). */
    multiboot2_set_info(mb_info_addr);

    /* Инициализируем конфиг ядра (hostname, цвета, размеры экрана). */
    config_init();

    /* Настраиваем консоль: framebuffer 1920x1080 при наличии, иначе VGA 80x25. */
    const kernel_config_t *cfg = config_get();
    vga_init(cfg->fg, cfg->bg);

    /* Инициализация простого аллокатора страниц от конца ядра. */
    paging_init();

    /* Инициализация heap (kmalloc/kfree). */
    heap_init();

    /* Инициализация IDT и обработчиков исключений. */
    idt_init();

    /* Минимальная таблица процессов (PID 1). */
    process_init();

    /* GDT/TSS и SYSCALL (User/Kernel разделение). */
    gdt_init();
    
    /* После инициализации всё лишнее не печатаем — сразу чистый экран и shell. */
    vga_clear();

    /* Инициализация простого in-memory FS. */
    fs_init();

    shell_run();
}

