#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* Размер страницы по умолчанию. */
#define PAGE_SIZE 4096

void paging_init(void);
void *alloc_page(void);
uint64_t paging_get_next_free(void);

#endif /* PAGING_H */

