#include "pic.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_ICW4      0x01
#define ICW1_INIT      0x10
#define ICW4_8086      0x01

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

void pic_init(void) {
    /* ICW1: начать инициализацию */
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    /* ICW2: базовый вектор для master (32) и slave (40) */
    outb(PIC1_DATA, 32);
    outb(PIC2_DATA, 40);

    /* ICW3: master имеет slave на IRQ2, slave — cascade */
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    /* Маски: разрешаем только IRQ0 (PIT) и IRQ1 (клавиатура) */
    outb(PIC1_DATA, 0xFC);  /* 0b11111100 */
    outb(PIC2_DATA, 0xFF);  /* все slave отключены */
}

void pic_eoi(int irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);
}
