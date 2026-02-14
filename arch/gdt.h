#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Инициализация GDT/TSS и SYSCALL.
 * Вызвать после idt_init(). Устанавливает TSS.RSP0, загружает TR, настраивает MSR. */
void gdt_init(void);

/* Обновить RSP0 в TSS (при переключении процесса — вызывать перед iret/sysret). */
void tss_set_rsp0(uint64_t rsp);

#endif /* GDT_H */
