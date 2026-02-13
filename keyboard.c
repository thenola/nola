#include "keyboard.h"
#include "vga.h"

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

#define KBD_STATUS_PORT 0x64
#define KBD_DATA_PORT   0x60

/* Простая раскладка для scancode set 1 (без Shift). */
static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
   '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'', '`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0,   0,   0,   ' ',
};

void keyboard_init(void) {
    /* Можно было бы очистить буфер контроллера; для простоты просто читаем порты. */
    (void)inb(KBD_STATUS_PORT);
    (void)inb(KBD_DATA_PORT);
}

static uint8_t read_scancode(void) {
    while (1) {
        uint8_t status = inb(KBD_STATUS_PORT);
        if (status & 0x01) {
            return inb(KBD_DATA_PORT);
        }
    }
}

char keyboard_getchar(void) {
    while (1) {
        uint8_t sc = read_scancode();

        /* Игнорируем break-коды (бит 7). */
        if (sc & 0x80) {
            continue;
        }

        if (sc < sizeof(keymap) && keymap[sc] != 0) {
            return keymap[sc];
        }
    }
}

void keyboard_read_line(char *buf, uint64_t max_len) {
    uint64_t len = 0;

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putc('\n');
            buf[len] = '\0';
            return;
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                vga_putc('\b');
            }
        } else {
            if (len + 1 < max_len) {
                buf[len++] = c;
                vga_putc(c);
            }
        }
    }
}

