TARGET   := kernel.elf
ISO      := kernel.iso

CC       ?= x86_64-elf-gcc
AS       := nasm

# Пути поиска заголовков
INCLUDES := -Ikernel -Idrivers -Iarch -Imm -Ifs -Ishell -Ilib

# Важно для обработчиков прерываний: отключаем red zone и SSE,
# чтобы GCC не пытался использовать SSE-инструкции в ISR.
# Также отключаем PIE, так как ядро должно быть не-PIE.
CFLAGS   := -m64 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow $(INCLUDES)
LDFLAGS  := -T linker.ld -nostdlib -z max-page-size=0x1000 -no-pie
ASFLAGS  := -f elf64

SRCS_C   := kernel/kernel.c drivers/vga.c drivers/keyboard.c arch/idt.c mm/paging.c mm/heap.c \
            lib/multiboot2.c lib/config.c shell/shell.c fs/fs.c
SRCS_ASM := boot/boot.asm boot/long_mode_init.asm boot/gdt.asm

OBJS     := kernel/kernel.o drivers/vga.o drivers/keyboard.o arch/idt.o mm/paging.o mm/heap.o \
            lib/multiboot2.o lib/config.o shell/shell.o fs/fs.o \
            boot/boot.o boot/long_mode_init.o boot/gdt.o

.PHONY: all clean run debug

all: $(ISO)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

$(TARGET): $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(ISO): $(TARGET) grub.cfg
	rm -rf isodir
	mkdir -p isodir/boot/grub
	cp $(TARGET) isodir/boot/kernel.elf
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO)

# Масштабирование под большой экран (1920x1080). Требует QEMU с GTK.
run-scaled: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -display gtk,zoom-to-fit=on

debug: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -s -S

clean:
	rm -rf $(OBJS) $(TARGET) $(ISO) isodir
