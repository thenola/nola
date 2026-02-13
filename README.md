## Nola 64-bit kernel (Multiboot2)

Простой 64-битный ядро/загрузчик для запуска под GRUB2 и QEMU.

### Структура проекта

- `boot.asm` — multiboot2-заголовок и 32-битная точка входа, передаёт управление в инициализацию long mode.
- `long_mode_init.asm` — проверка поддержки long mode, настройка PML4/PDPT/PD/PT и переход в 64-битный режим.
- `gdt.asm` — GDT с 64-битными сегментами (code/data) и заделом под TSS.
- `kernel.c` — основное ядро на C (`kernel_main`).
- `vga.c/.h` — простой драйвер текстового VGA (0xB8000).
- `idt.c/.h` — минимальная IDT и обработчик прерываний-заглушка.
- `paging.c/.h` — простой страничный аллокатор на основе символа `end` из линкера.
- `linker.ld` — 64-битный линкер-скрипт для ELF ядра с multiboot2-заголовком.
- `grub.cfg` — конфигурация GRUB2 для multiboot2-загрузки ядра.
- `Makefile` — сборка ядра, ISO-образа и запуск в QEMU.

### Установка пакетов в WSL (Ubuntu)

```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 grub-pc-bin xorriso
```

По умолчанию `Makefile` использует `x86_64-elf-gcc`. Если у вас нет кросс-компилятора,
можно использовать системный компилятор:

```bash
make CC=gcc
```

### Сборка и запуск

```bash
make          # собирает kernel.elf и kernel.iso
make run      # запускает QEMU: qemu-system-x86_64 -cdrom kernel.iso
```

Для отладки через GDB:

```bash
make debug    # QEMU с -s -S
```

Внутри QEMU вы должны увидеть строку:

- `Hello from 64-bit kernel!`
- информацию о размере указателя и селекторе CS,
- строку с вендором CPU,
- сообщения от аллокатора страниц и инициализации IDT.

### Сборка в Docker

Для удобства есть контейнер со всеми зависимостями (`buildenv/Dockerfile`).

Сборка образа:

```bash
docker build -t nola-build -f buildenv/Dockerfile .
```

Сборка ядра внутри контейнера (в корне проекта):

```bash
docker run --rm -it \
  -v "$PWD":/workspace \
  -w /workspace \
  nola-build \
  make CC=gcc
```

После этого в каталоге появится `kernel.iso`, который можно запустить обычным QEMU на хосте:

```bash
qemu-system-x86_64 -cdrom kernel.iso
```

