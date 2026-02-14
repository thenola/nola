#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROC_MAX 64

/* Минимальная структура процесса. */
struct process {
    uint64_t pid;
    uint64_t rsp;       /* kernel stack для переключения */
    uint8_t  state;     /* 0=free, 1=running, 2=zombie */
};

/* Текущий процесс (NULL = kernel/idle). */
struct process *process_current(void);

/* Инициализация: создать процесс 1 (init). */
void process_init(void);

/* Следующий свободный PID. */
uint64_t process_next_pid(void);

#endif /* PROCESS_H */
