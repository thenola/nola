#include "gdt.h"
#include <stdint.h>

/* TSS: RSP0 at offset 4 (64-bit). TSS descriptor: base at bytes 2-7, 8-11. */
#define TSS_RSP0_OFFSET 4

/* Внешние символы из asm. */
extern uint8_t tss64[];
extern uint8_t tss_descriptor[];
extern uint8_t stack_top[];

/* Заполнить TSS дескриптор в GDT (база = адрес tss64). */
static void tss_descriptor_set_base(void) {
    uint64_t base = (uint64_t)tss64;
    uint8_t *d = tss_descriptor;
    d[2] = (uint8_t)(base & 0xFF);
    d[3] = (uint8_t)((base >> 8) & 0xFF);
    d[4] = (uint8_t)((base >> 16) & 0xFF);
    d[7] = (uint8_t)((base >> 24) & 0xFF);
    *(uint32_t *)(d + 8) = (uint32_t)(base >> 32);
}

static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t lo = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t hi = (uint32_t)(value >> 32);
    __asm__ volatile("wrmsr" :: "c"(msr), "a"(lo), "d"(hi));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

#define MSR_EFER   0xC0000080
#define MSR_STAR   0xC0000081
#define MSR_LSTAR  0xC0000082
#define MSR_FMASK  0xC0000084
#define EFER_SCE   (1 << 0)

/* Селекторы из gdt.asm */
#define SEL_KERNEL_CS  0x08
#define SEL_USER_CS    0x18
#define SEL_TSS        0x28

void tss_set_rsp0(uint64_t rsp) {
    *(uint64_t *)(tss64 + TSS_RSP0_OFFSET) = rsp;
}

void gdt_init(void) {
    /* Заполнить базу TSS в дескрипторе GDT */
    tss_descriptor_set_base();

    /* TSS.RSP0 = вершина kernel stack */
    tss_set_rsp0((uint64_t)stack_top);

    /* Загрузить TR селектором TSS */
    __asm__ volatile("ltr %w0" :: "r"(SEL_TSS));

    /* Включить SYSCALL в EFER */
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | EFER_SCE);

    /* IA32_STAR: bits 47:32 = kernel CS (для SYSCALL), bits 63:48 = user CS (для SYSRET).
     * SS = CS + 8. Kernel: CS=0x08, SS=0x10. User: CS=0x18, SS=0x20. */
    wrmsr(MSR_STAR, ((uint64_t)SEL_USER_CS << 48) | ((uint64_t)SEL_KERNEL_CS << 32));

    /* IA32_LSTAR — адрес точки входа syscall (устанавливается в syscall.asm) */
    extern void syscall_entry(void);
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    /* IA32_FMASK: маскировать IF и DF при входе в kernel */
    wrmsr(MSR_FMASK, 0x600);  /* IF | DF */
}
