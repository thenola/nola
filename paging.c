#include "paging.h"
#include "vga.h"

/* Символ end определяется в линкер-скрипте и указывает на конец бинарника. */
extern uint8_t end;

static uint8_t *next_free_page = 0;

void paging_init(void) {
    /* Выравниваем следующий свободный адрес по границе страницы. */
    uint64_t addr = (uint64_t)&end;
    addr = (addr + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
    next_free_page = (uint8_t *)addr;

    vga_print("Paging allocator initialized at address ");
    vga_print_hex64((uint64_t)next_free_page);
    vga_putc('\n');
}

void *alloc_page(void) {
    void *page = next_free_page;
    next_free_page += PAGE_SIZE;

    vga_print("Allocated page at ");
    vga_print_hex64((uint64_t)page);
    vga_putc('\n');

    return page;
}

