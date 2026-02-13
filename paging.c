#include "paging.h"
#include "multiboot2.h"
#include "vga.h"

/* Символ end определяется в линкер-скрипте и указывает на конец бинарника. */
extern uint8_t end;

/* Физический адрес, по которому загружается ядро (см. linker.ld). */
#define KERNEL_PHYS_BASE 0x00100000ull

/* Максимальный физический адрес для bitmap (ограничиваем до 4 GiB для простоты). */
#define MAX_PHYS_ADDR_LIMIT 0x100000000ull

/* Информация о фреймах. */
static uint64_t g_max_phys_addr = 0;
static uint64_t g_frame_count   = 0;
static uint8_t *g_frame_bitmap  = 0; /* 1 = занято, 0 = свободно */

static uint32_t align_up32(uint32_t v, uint32_t a) {
    return (v + a - 1u) & ~(a - 1u);
}

static inline void frame_set_used(uint64_t frame_index) {
    if (!g_frame_bitmap || frame_index >= g_frame_count) {
        return;
    }
    uint64_t byte = frame_index / 8;
    uint8_t bit   = (uint8_t)(frame_index % 8);
    if (byte < (g_frame_count + 7) / 8) {
        g_frame_bitmap[byte] |= (uint8_t)(1u << bit);
    }
}

static inline void frame_set_free(uint64_t frame_index) {
    if (!g_frame_bitmap || frame_index >= g_frame_count) {
        return;
    }
    uint64_t byte = frame_index / 8;
    uint8_t bit   = (uint8_t)(frame_index % 8);
    if (byte < (g_frame_count + 7) / 8) {
        g_frame_bitmap[byte] &= (uint8_t)~(1u << bit);
    }
}

static inline int frame_is_free(uint64_t frame_index) {
    if (!g_frame_bitmap || frame_index >= g_frame_count) {
        return 0;
    }
    uint64_t byte = frame_index / 8;
    uint8_t bit   = (uint8_t)(frame_index % 8);
    if (byte >= (g_frame_count + 7) / 8) {
        return 0;
    }
    return (g_frame_bitmap[byte] & (uint8_t)(1u << bit)) == 0;
}

static void init_frame_bitmap(uint64_t mb_info_addr) {
    uint8_t *base = (uint8_t *)(uintptr_t)mb_info_addr;
    uint32_t total_size = *(uint32_t *)base;
    struct multiboot_tag *tag =
        (struct multiboot_tag *)(base + 8); /* после total_size и reserved */

    struct multiboot_tag_mmap *mmap_tag = 0;

    /* Находим карту памяти и максимальный адрес. */
    while ((uint8_t *)tag < base + total_size && tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap_tag = (struct multiboot_tag_mmap *)tag;
            uint8_t *entry_ptr = (uint8_t *)mmap_tag + sizeof(struct multiboot_tag_mmap);
            uint8_t *mmap_end  = (uint8_t *)mmap_tag + mmap_tag->size;

            while (entry_ptr < mmap_end) {
                struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;
                uint64_t end_addr = entry->addr + entry->len;
                /* Ограничиваем максимальный адрес до разумного значения. */
                if (end_addr > g_max_phys_addr && end_addr <= MAX_PHYS_ADDR_LIMIT) {
                    g_max_phys_addr = end_addr;
                } else if (end_addr > MAX_PHYS_ADDR_LIMIT && g_max_phys_addr < MAX_PHYS_ADDR_LIMIT) {
                    g_max_phys_addr = MAX_PHYS_ADDR_LIMIT;
                }
                entry_ptr += mmap_tag->entry_size;
            }
        }

        uint32_t size = tag->size;
        uint32_t next = (uint32_t)((uint8_t *)tag - base) + align_up32(size, 8u);
        tag = (struct multiboot_tag *)(base + next);
    }

    if (!mmap_tag || g_max_phys_addr == 0) {
        vga_println("Frame allocator: no memory map, falling back to dummy allocator.");
        return;
    }

    /* Ограничиваем максимальный адрес до 4 GiB. */
    if (g_max_phys_addr > MAX_PHYS_ADDR_LIMIT) {
        g_max_phys_addr = MAX_PHYS_ADDR_LIMIT;
    }

    g_frame_count = g_max_phys_addr / PAGE_SIZE;
    
    /* Ограничиваем количество фреймов разумным максимумом (например, 1M фреймов = 4GB). */
    if (g_frame_count > 0x100000) {
        g_frame_count = 0x100000;
        g_max_phys_addr = g_frame_count * PAGE_SIZE;
    }
    
    uint64_t bitmap_bytes = (g_frame_count + 7ull) / 8ull;

    /* Проверяем, что bitmap поместится в первые 2 MiB (отображенная память). */
    uint64_t bitmap_addr = (uint64_t)&end;
    bitmap_addr = (bitmap_addr + 7ull) & ~7ull; /* выравнивание по 8 байтам */
    
    /* Вычисляем максимальный размер bitmap, который поместится в отображенную память. */
    uint64_t mapped_memory_end = KERNEL_PHYS_BASE + 0x200000ull; /* конец первых 2 MiB */
    uint64_t max_bitmap_size = 0;
    if (bitmap_addr < mapped_memory_end) {
        max_bitmap_size = mapped_memory_end - bitmap_addr;
    } else {
        vga_println("Frame allocator: kernel too large, bitmap outside mapped memory!");
        return;
    }
    
    /* Если bitmap слишком большой, ограничиваем количество фреймов. */
    if (bitmap_bytes > max_bitmap_size || bitmap_bytes > 0x100000) { /* максимум 1 MiB */
        uint64_t limit_bytes = max_bitmap_size;
        if (limit_bytes > 0x100000) {
            limit_bytes = 0x100000;
        }
        /* Ограничиваем количество фреймов так, чтобы bitmap поместился. */
        uint64_t max_frames = (limit_bytes * 8ull);
        if (max_frames > g_frame_count) {
            max_frames = g_frame_count;
        }
        g_frame_count = max_frames;
        bitmap_bytes = (g_frame_count + 7ull) / 8ull;
        g_max_phys_addr = g_frame_count * PAGE_SIZE;
        vga_print("Frame allocator: limiting to ");
        vga_print_uint64(g_frame_count);
        vga_println(" frames due to memory constraints.");
    }

    g_frame_bitmap = (uint8_t *)bitmap_addr;

    /* Изначально помечаем все фреймы как занятые. */
    /* Используем актуальное значение bitmap_bytes после всех ограничений. */
    for (uint64_t i = 0; i < bitmap_bytes; ++i) {
        g_frame_bitmap[i] = 0xFF;
    }

    /* Помечаем usable RAM как свободную. */
    uint8_t *entry_ptr = (uint8_t *)mmap_tag + sizeof(struct multiboot_tag_mmap);
    uint8_t *mmap_end  = (uint8_t *)mmap_tag + mmap_tag->size;

    while (entry_ptr < mmap_end) {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;

        if (entry->type == 1) { /* usable */
            uint64_t start = entry->addr;
            uint64_t end   = entry->addr + entry->len;

            /* Выравниваем по границе страницы. */
            if (start % PAGE_SIZE) {
                start = (start + PAGE_SIZE - 1ull) & ~(PAGE_SIZE - 1ull);
            }
            end &= ~(PAGE_SIZE - 1ull);

            /* Ограничиваем обработку до MAX_PHYS_ADDR_LIMIT. */
            if (end > MAX_PHYS_ADDR_LIMIT) {
                end = MAX_PHYS_ADDR_LIMIT;
            }
            if (start >= end) {
                continue;
            }
            
            for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
                uint64_t frame = addr / PAGE_SIZE;
                if (frame < g_frame_count) {
                    frame_set_free(frame);
                }
            }
        }

        entry_ptr += mmap_tag->entry_size;
    }

    /* Резервируем память, занятую ядром и битовой картой. */
    uint64_t kernel_start = KERNEL_PHYS_BASE;
    uint64_t kernel_end   = bitmap_addr + bitmap_bytes;
    if (kernel_end % PAGE_SIZE) {
        kernel_end = (kernel_end + PAGE_SIZE - 1ull) & ~(PAGE_SIZE - 1ull);
    }

    for (uint64_t addr = kernel_start; addr < kernel_end; addr += PAGE_SIZE) {
        uint64_t frame = addr / PAGE_SIZE;
        if (frame < g_frame_count) {
            frame_set_used(frame);
        }
    }

    vga_print("Frame bitmap at ");
    vga_print_hex64(bitmap_addr);
    vga_print(", frames total: ");
    vga_print_uint64(g_frame_count);
    vga_putc('\n');
}

void paging_init(uint64_t mb_info_addr) {
    init_frame_bitmap(mb_info_addr);
}

void *alloc_page(void) {
    if (!g_frame_bitmap || g_frame_count == 0) {
        vga_println("alloc_page: frame allocator not initialized.");
        return 0;
    }

    /* Ограничиваем поиск разумным количеством фреймов. */
    uint64_t max_search = g_frame_count;
    if (max_search > 0x10000) { /* максимум 65536 фреймов для поиска */
        max_search = 0x10000;
    }

    for (uint64_t i = 0; i < max_search; ++i) {
        if (frame_is_free(i)) {
            frame_set_used(i);
            uint64_t addr = i * PAGE_SIZE;
            vga_print("Allocated phys frame at ");
            vga_print_hex64(addr);
            vga_putc('\n');
            return (void *)(uintptr_t)addr;
        }
    }

    vga_println("alloc_page: out of physical memory!");
    return 0;
}

