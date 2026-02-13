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

/* Простой дамп основных тегов Multiboot2 на экран (этап 1 плана). */
void multiboot2_dump_info(uint32_t magic, uint64_t info_addr);

#endif /* MULTIBOOT2_H */

