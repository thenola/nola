#include "idt.h"
#include "vga.h"

/* Глобальный IDT. */
static struct idt_entry idt[IDT_MAX_ENTRIES];

static inline void lidt(struct idt_ptr *idtr) {
    __asm__ volatile("lidt (%0)" :: "r"(idtr));
}

static void idt_set_gate(int num, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;

    idt[num].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[num].selector    = 0x08;      /* 64-bit code segment selector */
    idt[num].ist         = 0;
    idt[num].type_attr   = 0x8E;      /* interrupt gate, present */
    idt[num].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[num].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[num].zero        = 0;
}

/* Простой обработчик-заглушка. Прерывания мы не включаем, так что это только демонстрация. */
static void isr_default(void) {
    vga_println("Interrupt or exception occurred (stub handler).");
}

void idt_init(void) {
    /* Обнуляем таблицу. */
    for (int i = 0; i < IDT_MAX_ENTRIES; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].zero        = 0;
    }

    /* Для простоты все первые 32 вектора (исключения) указывают на один обработчик-заглушку. */
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, isr_default);
    }

    struct idt_ptr idtr;
    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base  = (uint64_t)&idt[0];

    lidt(&idtr);
}
