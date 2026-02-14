## Nola 64-bit kernel 
<img width="843" height="569" alt="{06DC130C-0EF4-4774-A45E-4A2943527AB8}" src="https://github.com/user-attachments/assets/ce7be6d9-6bd2-4a10-8176-e597214fb2f6" />

### Структура проекта

```
nola/
├── boot/       — загрузчик и ассемблер (boot.asm, long_mode_init.asm, gdt.asm)
├── kernel/     — ядро (kernel.c)
├── drivers/    — драйверы (VGA, клавиатура)
├── arch/       — архитектурно-зависимый код (IDT)
├── mm/         — управление памятью (paging)
├── fs/         — файловая система in-memory
├── shell/      — командная оболочка
├── lib/        — multiboot2, config, user
├── buildenv/   — Docker-среда сборки
├── linker.ld   — линкер-скрипт
├── grub.cfg    — конфиг GRUB2
└── Makefile
```

### Build

```bash
@echo off
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make clean
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make CC=gcc
"C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom kernel.iso

```
