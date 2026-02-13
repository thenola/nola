TARGET   := kernel.elf
ISO      := kernel.iso

CC       ?= x86_64-elf-gcc
AS       := nasm

# Важно для обработчиков прерываний: отключаем red zone и SSE,
# чтобы GCC не пытался использовать SSE-инструкции в ISR.
# Также отключаем PIE, так как ядро должно быть не-PIE.
CFLAGS   := -m64 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow
LDFLAGS  := -T linker.ld -nostdlib -z max-page-size=0x1000 -no-pie
ASFLAGS  := -f elf64

SRCS_C   := kernel.c vga.c idt.c paging.c multiboot2.c
SRCS_ASM := boot.asm long_mode_init.asm gdt.asm

OBJS     := $(SRCS_C:.c=.o) $(SRCS_ASM:.asm=.o)

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

debug: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -s -S

clean:
	rm -rf $(OBJS) $(TARGET) $(ISO) isodir

