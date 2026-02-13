#include <stdint.h>
#include <stddef.h>

#include "vga.h"
#include "idt.h"
#include "paging.h"

/* Простой cpuid-хелпер. */
static void cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(leaf), "c"(0));
    if (a) *a = eax;
    if (b) *b = ebx;
    if (c) *c = ecx;
    if (d) *d = edx;
}

static void print_cpu_info(void) {
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    cpuid(0, &eax, &ebx, &ecx, &edx);

    vendor[0]  = (char)(ebx & 0xFF);
    vendor[1]  = (char)((ebx >> 8) & 0xFF);
    vendor[2]  = (char)((ebx >> 16) & 0xFF);
    vendor[3]  = (char)((ebx >> 24) & 0xFF);
    vendor[4]  = (char)(edx & 0xFF);
    vendor[5]  = (char)((edx >> 8) & 0xFF);
    vendor[6]  = (char)((edx >> 16) & 0xFF);
    vendor[7]  = (char)((edx >> 24) & 0xFF);
    vendor[8]  = (char)(ecx & 0xFF);
    vendor[9]  = (char)((ecx >> 8) & 0xFF);
    vendor[10] = (char)((ecx >> 16) & 0xFF);
    vendor[11] = (char)((ecx >> 24) & 0xFF);
    vendor[12] = '\0';

    vga_print("CPU vendor: ");
    vga_println(vendor);
}

static void check_64bit_mode(void) {
    uint16_t cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));

    vga_print("Current CS selector: 0x");
    vga_print_hex64((uint64_t)cs);
    vga_putc('\n');

    vga_print("Size of pointer: ");
    vga_print_uint64((uint64_t)sizeof(void *));
    vga_println(" bytes");
}

void kernel_main(void) {
    vga_init(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    vga_println("Hello from 64-bit kernel!");
    vga_println("---------------------------");

    /* Демонстрация 64-бит режима. */
    check_64bit_mode();

    /* Информация о процессоре. */
    print_cpu_info();

    /* Инициализация простого аллокатора страниц. */
    paging_init();
    void *page1 = alloc_page();
    (void)page1;

    /* Инициализация IDT и базовых обработчиков прерываний. */
    idt_init();
    vga_println("IDT initialized.");

    vga_println("Kernel initialization complete. System is running.");

    for (;;) {
        __asm__ volatile("hlt");
    }
}

