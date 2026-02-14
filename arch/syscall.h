#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/* Номера syscall (Linux-совместимые). */
#define SYS_exit   60
#define SYS_read   63
#define SYS_write  64
#define SYS_getpid 39

/* Коды ошибок. */
#define EINVAL 22
#define EBADF  9

/* Диспетчер: вызывается из syscall_entry. */
uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5);

#endif /* SYSCALL_H */
