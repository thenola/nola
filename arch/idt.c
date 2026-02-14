#include "idt.h"
#include "vga.h"
#include "cpu.h"
#include <stdint.h>

/* Глобальный IDT. */
static struct idt_entry idt[IDT_MAX_ENTRIES];

/* Внешние обработчики из isr.asm */
extern void isr0(void), isr1(void), isr2(void), isr3(void), isr4(void), isr5(void), isr6(void), isr7(void);
extern void isr8(void), isr9(void), isr10(void), isr11(void), isr12(void), isr13(void), isr14(void), isr15(void);
extern void isr16(void), isr17(void), isr18(void), isr19(void), isr20(void), isr21(void), isr22(void), isr23(void);
extern void isr24(void), isr25(void), isr26(void), isr27(void), isr28(void), isr29(void), isr30(void), isr31(void);

static inline void lidt(struct idt_ptr *idtr) {
    __asm__ volatile("lidt (%0)" :: "r"(idtr));
}

static void idt_set_gate(int num, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;

    idt[num].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[num].selector    = 0x08;
    idt[num].ist         = 0;
    idt[num].type_attr   = 0x8E;
    idt[num].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[num].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[num].zero        = 0;
}

static const char *exceptions[] = {
    "Divide by zero", "Debug", "NMI", "Breakpoint", "Overflow", "Bounds",
    "Invalid opcode", "Device not available", "Double fault", "Coprocessor segment overrun",
    "Invalid TSS", "Segment not present", "Stack fault", "General protection",
    "Page fault", "Reserved", "x87 FPU", "Alignment check", "Machine check",
    "SIMD exception", "Virtualization", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Security", "Reserved"
};

void idt_handler(uint64_t vector, uint64_t error_code) {
    if (vector < 32) {
        vga_print("Exception ");
        vga_print_uint64(vector);
        vga_print(": ");
        vga_println(exceptions[vector]);
        if (vector == 14) {
            vga_print("  CR2=");
            uint64_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            vga_print_hex64(cr2);
            vga_print(" error=");
            vga_print_hex64(error_code);
            vga_putc('\n');
        }
        cpu_halt();
    }
}

void idt_init(void) {
    for (int i = 0; i < IDT_MAX_ENTRIES; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].zero        = 0;
    }

    void (*handlers[])(void) = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
        isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, handlers[i]);
    }

    struct idt_ptr idtr;
    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base  = (uint64_t)&idt[0];

    lidt(&idtr);
}
