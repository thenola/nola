#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

/* Magic значение, которое GRUB передаёт в EAX для Multiboot2. */
#define MULTIBOOT2_MAGIC 0x36D76289u

/* Типы тегов Multiboot2 (не полный список, только то, что нужно ядру). */
enum {
    MULTIBOOT_TAG_TYPE_END              = 0,
    MULTIBOOT_TAG_TYPE_CMDLINE          = 1,
    MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME = 2,
    MULTIBOOT_TAG_TYPE_MODULE           = 3,
    MULTIBOOT_TAG_TYPE_BASIC_MEMINFO    = 4,
    MULTIBOOT_TAG_TYPE_BOOTDEV          = 5,
    MULTIBOOT_TAG_TYPE_MMAP             = 6,
};

/* Общий заголовок тега. */
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

/* Тег с картой памяти. */
struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* Далее идут записи struct multiboot_mmap_entry[...] */
} __attribute__((packed));

/* Запись карты памяти. */
struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

/* Тег модуля. */
struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char string[];
} __attribute__((packed));

void multiboot2_dump_info(uint32_t magic, uint64_t info_addr);

/* Сохранить адрес multiboot info для последующего парсинга. */
void multiboot2_set_info(uint64_t info_addr);

/* Получить модуль по индексу (0-based). Возвращает 0 при успехе. */
int multiboot2_get_module(int index, uint64_t *start, uint64_t *end);

#endif /* MULTIBOOT2_H */

