#include "heap.h"
#include "paging.h"
#include <stddef.h>
#include <stdint.h>

#define ALIGN           8
#define MIN_BLOCK_SIZE  32   /* header 16 + min payload 16 */
#define SPLIT_THRESH    64   /* split if remainder >= this */

typedef struct heap_block {
    size_t size;                    /* полный размер блока включая заголовок */
    struct heap_block *next;        /* следующий в списке свободных (если свободен) */
} heap_block_t;

static heap_block_t *free_list = 0;

static size_t align_up(size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

static void coalesce(heap_block_t *block) {
    uintptr_t block_end = (uintptr_t)block + block->size;

    /* Ищем следующий блок в памяти (должен начинаться сразу после нас) */
    heap_block_t **p = &free_list;
    while (*p) {
        heap_block_t *b = *p;
        if ((uintptr_t)b == block_end) {
            /* Сливаем с блоком справа */
            block->size += b->size;
            block->next = b->next;
            *p = block;
            /* Рекурсивно — возможно есть ещё блоки справа */
            coalesce(block);
            return;
        }
        if ((uintptr_t)b + b->size == (uintptr_t)block) {
            /* b заканчивается там, где начинается block — сливаем в b */
            b->size += block->size;
            coalesce(b);
            return;
        }
        p = &(*p)->next;
    }

    /* Вставляем в свободный список (сортируем по адресу для coalesce) */
    p = &free_list;
    while (*p && (uintptr_t)(*p) < (uintptr_t)block) {
        p = &(*p)->next;
    }
    block->next = *p;
    *p = block;
}

void heap_init(void) {
    void *page = alloc_page_silent();
    heap_block_t *block = (heap_block_t *)page;
    block->size = PAGE_SIZE;
    block->next = 0;
    free_list = block;
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;

    size_t total = align_up(size) + sizeof(heap_block_t);
    if (total < MIN_BLOCK_SIZE) total = MIN_BLOCK_SIZE;

    for (;;) {
        heap_block_t **prev = &free_list;
        heap_block_t *b = free_list;

        while (b) {
            if (b->size >= total) {
                /* Нашли подходящий блок */
                size_t rem = b->size - total;
                if (rem >= SPLIT_THRESH) {
                    /* Разбиваем: новый свободный блок справа */
                    heap_block_t *new_free = (heap_block_t *)((uint8_t *)b + total);
                    new_free->size = rem;
                    new_free->next = b->next;
                    b->size = total;
                    *prev = new_free;
                } else {
                    *prev = b->next;
                }
                b->next = 0;  /* занят */
                return (void *)((uint8_t *)b + sizeof(heap_block_t));
            }
            prev = &b->next;
            b = b->next;
        }

        /* Нет подходящего блока — берём новую страницу */
        void *page = alloc_page_silent();
        heap_block_t *new_block = (heap_block_t *)page;
        new_block->size = PAGE_SIZE;
        coalesce(new_block);
    }
}

void kfree(void *ptr) {
    if (!ptr) return;

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    block->next = 0;
    coalesce(block);
}
