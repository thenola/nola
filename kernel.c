#include <stdint.h>
#include <stddef.h>

#include "vga.h"
#include "idt.h"
#include "paging.h"
#include "multiboot2.h"
#include "shell.h"
#include "fs.h"
#include "user.h"
#include "config.h"

void kernel_main(uint32_t mb_magic, uint64_t mb_info_addr) {
    /* Сразу после загрузки очищаем экран и ставим белый текст на чёрном фоне. */
    (void)mb_magic;
    (void)mb_info_addr;

    /* Инициализируем конфиг ядра (hostname, цвета, размеры экрана). */
    config_init();

    /* Настраиваем VGA под конфиг. */
    const kernel_config_t *cfg = config_get();
    vga_init(cfg->fg, cfg->bg);

    /* Инициализация простого аллокатора страниц от конца ядра. */
    paging_init();

    /* Инициализация IDT и базовых обработчиков прерываний (без включения IRQ). */
    idt_init();
    
    /* После инициализации всё лишнее не печатаем — сразу чистый экран и shell. */
    vga_clear();

    /* Инициализация простого in-memory FS. */
    fs_init();

    /* Инициализация пользователей (root и user). */
    user_init();

    shell_run();
}

