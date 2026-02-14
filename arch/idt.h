#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* Максимальное количество записей в IDT. */
#define IDT_MAX_ENTRIES 256

/* Структура записи IDT для x86_64. */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void idt_init(void);

#endif /* IDT_H */

