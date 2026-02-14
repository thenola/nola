#include "syscall.h"
#include "vga.h"
#include "keyboard.h"
#include "process.h"
#include "cpu.h"
#include <stdint.h>

#define MAX_WRITE_LEN 4096

static int64_t do_write(uint64_t fd, const void *buf, uint64_t len) {
    if (len > MAX_WRITE_LEN)
        return -EINVAL;

    if (fd == 1 || fd == 2) {
        const char *p = (const char *)buf;
        for (uint64_t i = 0; i < len; i++)
            vga_putc(p[i]);
        return (int64_t)len;
    }
    return -EBADF;
}

static int64_t do_read(uint64_t fd, void *buf, uint64_t count) {
    if (fd != 0)
        return -EBADF;
    if (count == 0)
        return 0;
    if (count > MAX_WRITE_LEN)
        count = MAX_WRITE_LEN;

    char *p = (char *)buf;
    uint64_t n = 0;
    while (n < count) {
        char c = keyboard_getchar();
        p[n++] = c;
        if (c == '\n')
            break;
    }
    return (int64_t)n;
}

uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5) {
    (void)a4;
    (void)a5;

    switch (num) {
    case SYS_getpid: {
        struct process *p = process_current();
        return p ? p->pid : 0;
    }

    case SYS_write:
        return (uint64_t)(int64_t)do_write(a1, (const void *)a2, a3);

    case SYS_read:
        return (uint64_t)(int64_t)do_read(a1, (void *)a2, a3);

    case SYS_exit:
        cpu_halt();
        return 0;  /* не достигается */

    default:
        return (uint64_t)(int64_t)-EINVAL;
    }
}
