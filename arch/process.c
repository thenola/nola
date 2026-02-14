#include "process.h"
#include <stddef.h>

static struct process procs[PROC_MAX];
static struct process *current_proc;
static uint64_t next_pid = 1;

struct process *process_current(void) {
    return current_proc;
}

uint64_t process_next_pid(void) {
    return next_pid++;
}

void process_init(void) {
    for (int i = 0; i < PROC_MAX; i++) {
        procs[i].state = 0;
    }
    procs[0].pid = 1;
    procs[0].state = 1;
    procs[0].rsp = 0;
    current_proc = &procs[0];
}
