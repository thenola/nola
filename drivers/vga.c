#include "vga.h"
#include "multiboot2.h"
#include "font_8x16.h"
#include <stddef.h>
#include <stdint.h>

/* --- VGA text mode (80x25) --- */
static volatile uint16_t *const VGA_MEMORY = (uint16_t *)0xB8000;
static const size_t VGA_WIDTH  = 80;
static const size_t VGA_HEIGHT = 25;

static size_t cursor_row = 0;
static size_t cursor_col = 0;
static uint8_t current_color = 0;

/* --- Framebuffer mode (1920x1080) --- */
#define FONT_CELL_W  8
#define FONT_CELL_H  16   /* шрифт 8x16 (VGA) */
#define CHAR_SCALE   2    /* масштаб: каждый пиксель = 2x2 на экране */
#define CHAR_W       (FONT_CELL_W * CHAR_SCALE)   /* 16 */
#define CHAR_H       (FONT_CELL_H * CHAR_SCALE)   /* 32 */

static int use_fb = 0;
static volatile uint8_t *fb_base = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t fb_bpp = 0;
static uint32_t fb_cols = 0;
static uint32_t fb_rows = 0;
static uint8_t fb_red_shift = 0, fb_red_size = 8;
static uint8_t fb_green_shift = 0, fb_green_size = 8;
static uint8_t fb_blue_shift = 0, fb_blue_size = 8;
static uint32_t fb_fg = 0x00FFFFFF;
static uint32_t fb_bg = 0x00000000;

/* VGA color index -> RGB (for framebuffer) */
static const uint32_t vga_to_rgb[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

static uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)(fg | (bg << 4));
}

static uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | ((uint16_t)color << 8);
}

static uint32_t fb_make_pixel(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t pix = 0;
    if (fb_red_size)   pix |= ((uint32_t)(r >> (8 - fb_red_size))   << fb_red_shift);
    if (fb_green_size) pix |= ((uint32_t)(g >> (8 - fb_green_size)) << fb_green_shift);
    if (fb_blue_size)  pix |= ((uint32_t)(b >> (8 - fb_blue_size))  << fb_blue_shift);
    return pix;
}

/* Пиксель в формате 0x00RRGGBB. Записываем байты в порядке R,G,B. */
static void fb_put_pixel(uint32_t x, uint32_t y, uint32_t pixel) {
    uint8_t r = (uint8_t)(pixel >> 16);
    uint8_t g = (uint8_t)(pixel >> 8);
    uint8_t b = (uint8_t)(pixel);
    if (fb_bpp == 32) {
        volatile uint8_t *p = fb_base + (uint64_t)y * fb_pitch + (uint64_t)x * 4;
        p[0] = r;
        p[1] = g;
        p[2] = b;
        p[3] = 0;
    } else if (fb_bpp == 24) {
        volatile uint8_t *p = fb_base + (uint64_t)y * fb_pitch + (uint64_t)x * 3;
        p[0] = r;
        p[1] = g;
        p[2] = b;
    }
}

static void fb_draw_char(uint32_t px, uint32_t py, unsigned char c, uint32_t fg, uint32_t bg) {
    unsigned int ch = (unsigned int)(c & 0x7F);
    const unsigned char *glyph = font8x16[ch];
    for (uint32_t row = 0; row < FONT_CELL_H; row++) {
        unsigned char line = glyph[row];
        for (uint32_t col = 0; col < FONT_CELL_W; col++) {
            /* MSB = слева (VGA 8x16) */
            uint32_t color = (line & (0x80u >> col)) ? fg : bg;
            uint32_t x0 = px + col * CHAR_SCALE;
            uint32_t y0 = py + row * CHAR_SCALE;
            for (uint32_t sy = 0; sy < CHAR_SCALE; sy++)
                for (uint32_t sx = 0; sx < CHAR_SCALE; sx++)
                    fb_put_pixel(x0 + sx, y0 + sy, color);
        }
    }
}

static void fb_clear(void) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb_put_pixel(x, y, fb_bg);
        }
    }
}

static void fb_scroll(void) {
    uint32_t line_bytes = fb_pitch * CHAR_H;
    for (uint32_t y = 0; y < fb_height - CHAR_H; y++) {
        volatile uint8_t *dst = fb_base + y * fb_pitch;
        volatile uint8_t *src = fb_base + (y + CHAR_H) * fb_pitch;
        for (uint32_t i = 0; i < line_bytes; i++) {
            dst[i] = src[i];
        }
    }
    for (uint32_t y = fb_height - CHAR_H; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb_put_pixel(x, y, fb_bg);
        }
    }
}

static void fb_newline(void) {
    cursor_col = 0;
    cursor_row++;
    if (cursor_row >= fb_rows) {
        fb_scroll();
        cursor_row = fb_rows - 1;
    }
}

static void fb_putc(char c) {
    if (c == '\n') {
        fb_newline();
        return;
    }
    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = fb_cols - 1;
        }
        fb_draw_char(cursor_col * CHAR_W, cursor_row * CHAR_H, ' ', fb_fg, fb_bg);
        return;
    }
    fb_draw_char(cursor_col * CHAR_W, cursor_row * CHAR_H, (unsigned char)c, fb_fg, fb_bg);
    cursor_col++;
    if (cursor_col >= fb_cols) {
        fb_newline();
    }
}

void vga_init(vga_color_t fg, vga_color_t bg) {
    multiboot_fb_info_t fb_info;
    if (multiboot2_get_framebuffer(&fb_info) && fb_info.type == MULTIBOOT_FB_TYPE_RGB && fb_info.bpp >= 16) {
        use_fb = 1;
        fb_base = (volatile uint8_t *)(uintptr_t)fb_info.addr;
        fb_pitch = fb_info.pitch;
        fb_width = fb_info.width;
        fb_height = fb_info.height;
        fb_bpp = fb_info.bpp;
        fb_cols = fb_width / CHAR_W;
        fb_rows = fb_height / CHAR_H;
        fb_red_shift = fb_info.red_shift;
        fb_red_size = fb_info.red_mask ? (uint8_t)fb_info.red_mask : 8;
        fb_green_shift = fb_info.green_shift;
        fb_green_size = fb_info.green_mask ? (uint8_t)fb_info.green_mask : 8;
        fb_blue_shift = fb_info.blue_shift;
        fb_blue_size = fb_info.blue_mask ? (uint8_t)fb_info.blue_mask : 8;

        /* Белый текст: явно 0xAARRGGBB (R=G=B=255). Обход неверного формата из multiboot. */
        fb_fg = 0x00FFFFFF;
        fb_bg = 0x00000000;
        if (fg != VGA_COLOR_WHITE || bg != VGA_COLOR_BLACK) {
            uint32_t fg_rgb = vga_to_rgb[fg & 15];
            uint32_t bg_rgb = vga_to_rgb[bg & 15];
            fb_fg = fb_make_pixel((uint8_t)(fg_rgb >> 16), (uint8_t)(fg_rgb >> 8), (uint8_t)(fg_rgb));
            fb_bg = fb_make_pixel((uint8_t)(bg_rgb >> 16), (uint8_t)(bg_rgb >> 8), (uint8_t)(bg_rgb));
        }

        cursor_row = 0;
        cursor_col = 0;
        fb_clear();
        return;
    }

    use_fb = 0;
    current_color = vga_entry_color(fg, bg);
    cursor_row = 0;
    cursor_col = 0;
    vga_clear();
}

void vga_clear(void) {
    if (use_fb) {
        cursor_row = 0;
        cursor_col = 0;
        fb_clear();
        return;
    }
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
    if (use_fb) {
        uint32_t fg_rgb = vga_to_rgb[fg & 15];
        uint32_t bg_rgb = vga_to_rgb[bg & 15];
        fb_fg = fb_make_pixel((uint8_t)(fg_rgb >> 16), (uint8_t)(fg_rgb >> 8), (uint8_t)(fg_rgb));
        fb_bg = fb_make_pixel((uint8_t)(bg_rgb >> 16), (uint8_t)(bg_rgb >> 8), (uint8_t)(bg_rgb));
        return;
    }
    current_color = vga_entry_color(fg, bg);
}

static void vga_scroll(void) {
    if (cursor_row < VGA_HEIGHT) return;
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }
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
    if (cursor_row >= VGA_HEIGHT) vga_scroll();
}

void vga_putc(char c) {
    if (use_fb) {
        fb_putc(c);
        return;
    }
    if (c == '\n') {
        vga_newline();
        return;
    }
    if (c == '\b') {
        if (cursor_col > 0) cursor_col--;
        else if (cursor_row > 0) { cursor_row--; cursor_col = VGA_WIDTH - 1; }
        const size_t index = cursor_row * VGA_WIDTH + cursor_col;
        VGA_MEMORY[index] = vga_entry(' ', current_color);
        return;
    }
    const size_t index = cursor_row * VGA_WIDTH + cursor_col;
    VGA_MEMORY[index] = vga_entry((unsigned char)c, current_color);
    if (++cursor_col >= VGA_WIDTH) vga_newline();
}

void vga_print(const char *str) {
    while (*str) vga_putc(*str++);
}

void vga_println(const char *str) {
    vga_print(str);
    vga_putc('\n');
}

void vga_print_uint64(uint64_t value) {
    char buf[21];
    int i = 0;
    if (value == 0) { vga_putc('0'); return; }
    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (i-- > 0) vga_putc(buf[i]);
}

void vga_print_hex64(uint64_t value) {
    static const char *hex = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        vga_putc(hex[(value >> i) & 0xF]);
    }
}
