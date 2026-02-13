#include "vga.h"

/* Адрес текстового VGA-буфера. */
static volatile uint16_t *const VGA_MEMORY = (uint16_t *)0xB8000;

static const size_t VGA_WIDTH  = 80;
static const size_t VGA_HEIGHT = 25;

static size_t cursor_row = 0;
static size_t cursor_col = 0;
static uint8_t current_color = 0;

static uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)(fg | (bg << 4));
}

static uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | ((uint16_t)color << 8);
}

void vga_init(vga_color_t fg, vga_color_t bg) {
    current_color = vga_entry_color(fg, bg);
    cursor_row = 0;
    cursor_col = 0;
    vga_clear();
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_MEMORY[index] = vga_entry(' ', current_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    current_color = vga_entry_color(fg, bg);
}

static void vga_scroll(void) {
    if (cursor_row < VGA_HEIGHT) {
        return;
    }

    /* Сдвигаем все строки вверх на одну. */
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }

    /* Очищаем последнюю строку. */
    size_t y = VGA_HEIGHT - 1;
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', current_color);
    }

    cursor_row = VGA_HEIGHT - 1;
    cursor_col = 0;
}

static void vga_newline(void) {
    cursor_col = 0;
    cursor_row++;
    if (cursor_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }

    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = VGA_WIDTH - 1;
        }
        const size_t index = cursor_row * VGA_WIDTH + cursor_col;
        VGA_MEMORY[index] = vga_entry(' ', current_color);
        return;
    }

    const size_t index = cursor_row * VGA_WIDTH + cursor_col;
    VGA_MEMORY[index] = vga_entry((unsigned char)c, current_color);

    if (++cursor_col >= VGA_WIDTH) {
        vga_newline();
    }
}

void vga_print(const char *str) {
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_println(const char *str) {
    vga_print(str);
    vga_putc('\n');
}

void vga_print_uint64(uint64_t value) {
    /* Печать без выделения памяти: сначала собираем цифры в буфер в обратном порядке. */
    char buf[21];
    int i = 0;

    if (value == 0) {
        vga_putc('0');
        return;
    }

    while (value > 0 && i < (int)sizeof(buf)) {
        uint64_t digit = value % 10;
        buf[i++] = (char)('0' + digit);
        value /= 10;
    }

    while (i-- > 0) {
        vga_putc(buf[i]);
    }
}

void vga_print_hex64(uint64_t value) {
    static const char *hex = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (uint8_t)((value >> i) & 0xF);
        vga_putc(hex[nibble]);
    }
}

