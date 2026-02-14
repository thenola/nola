#ifndef CPU_H
#define CPU_H

/* Остановить CPU. */
void cpu_halt(void);

/* Перезагрузка через keyboard controller. */
void cpu_reboot(void);

#endif /* CPU_H */
