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
    MULTIBOOT_TAG_TYPE_VBE              = 7,
    MULTIBOOT_TAG_TYPE_FRAMEBUFFER      = 8,
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

/* Framebuffer: тип 1 = RGB. */
#define MULTIBOOT_FB_TYPE_INDEXED 0
#define MULTIBOOT_FB_TYPE_RGB     1
#define MULTIBOOT_FB_TYPE_EGA    2

typedef struct multiboot_fb_info {
    uint64_t addr;       /* физический адрес буфера */
    uint32_t pitch;      /* байт на строку */
    uint32_t width;      /* ширина в пикселях */
    uint32_t height;     /* высота в пикселях */
    uint8_t  bpp;        /* бит на пиксель */
    uint8_t  type;       /* 1 = RGB */
    uint8_t  red_shift;  /* сдвиг красного (RGB) */
    uint8_t  red_mask;
    uint8_t  green_shift;
    uint8_t  green_mask;
    uint8_t  blue_shift;
    uint8_t  blue_mask;
} multiboot_fb_info_t;

/* Сохранить адрес multiboot info; затем вызвать multiboot2_get_framebuffer. */
void multiboot2_set_info(uint64_t info_addr);

/* Заполняет *fb из тега type 8. Возвращает 1 если framebuffer есть, 0 иначе. */
int multiboot2_get_framebuffer(multiboot_fb_info_t *fb);

#endif /* MULTIBOOT2_H */

