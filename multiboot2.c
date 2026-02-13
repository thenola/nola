#include "multiboot2.h"
#include "vga.h"

static uint32_t align_up(uint32_t value, uint32_t align) {
    return (value + align - 1u) & ~(align - 1u);
}

void multiboot2_dump_info(uint32_t magic, uint64_t info_addr) {
    if (magic != MULTIBOOT2_MAGIC) {
        vga_println("Multiboot2: invalid magic, tags not parsed.");
        return;
    }

    if (info_addr == 0) {
        vga_println("Multiboot2: info address is 0.");
        return;
    }

    uint8_t *base = (uint8_t *)(uintptr_t)info_addr;
    uint32_t total_size = *(uint32_t *)base;
    (void)*(uint32_t *)(base + 4); /* reserved, не используем */

    vga_print("Multiboot2 total size: ");
    vga_print_uint64(total_size);
    vga_putc('\n');

    struct multiboot_tag *tag =
        (struct multiboot_tag *)(base + 8); /* после total_size и reserved */

    uint64_t total_usable_mem = 0;

    while ((uint8_t *)tag < base + total_size && tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE: {
            const char *cmdline = (const char *)((uint8_t *)tag + sizeof(struct multiboot_tag));
            vga_print("MB2 cmdline: ");
            vga_println(cmdline);
            break;
        }
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME: {
            const char *name = (const char *)((uint8_t *)tag + sizeof(struct multiboot_tag));
            vga_print("Boot loader: ");
            vga_println(name);
            break;
        }
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
            uint32_t mem_lower = *(uint32_t *)((uint8_t *)tag + sizeof(struct multiboot_tag));
            uint32_t mem_upper = *(uint32_t *)((uint8_t *)tag + sizeof(struct multiboot_tag) + 4);
            vga_print("Mem lower (KB): ");
            vga_print_uint64(mem_lower);
            vga_putc('\n');
            vga_print("Mem upper (KB): ");
            vga_print_uint64(mem_upper);
            vga_putc('\n');
            break;
        }
        case MULTIBOOT_TAG_TYPE_MMAP: {
            struct multiboot_tag_mmap *mmap_tag = (struct multiboot_tag_mmap *)tag;
            vga_println("Multiboot2 memory map:");

            uint8_t *entry_ptr = (uint8_t *)mmap_tag + sizeof(struct multiboot_tag_mmap);
            uint8_t *mmap_end = (uint8_t *)mmap_tag + mmap_tag->size;

            while (entry_ptr < mmap_end) {
                struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;

                if (entry->type == 1) { /* usable RAM */
                    total_usable_mem += entry->len;
                }

                vga_print("  region: base=");
                vga_print_hex64(entry->addr);
                vga_print(" length=");
                vga_print_hex64(entry->len);
                vga_print(" type=");
                vga_print_uint64(entry->type);
                vga_putc('\n');

                entry_ptr += mmap_tag->entry_size;
            }
            break;
        }
        default:
            /* Для остальных тегов пока просто ничего не делаем. */
            break;
        }

        uint32_t size = tag->size;
        uint32_t next = (uint32_t)((uint8_t *)tag - base) + align_up(size, 8u);
        tag = (struct multiboot_tag *)(base + next);
    }

    if (total_usable_mem != 0) {
        vga_print("Total usable RAM (bytes): ");
        vga_print_hex64(total_usable_mem);
        vga_putc('\n');
    }
}

