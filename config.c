#include "config.h"

static kernel_config_t cfg;

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static vga_color_t parse_color(const char *name, vga_color_t def) {
    if (!name || !*name) return def;
    while (*name == ' ') name++;
    if (str_eq(name, "black"))       return VGA_COLOR_BLACK;
    if (str_eq(name, "blue"))        return VGA_COLOR_BLUE;
    if (str_eq(name, "green"))       return VGA_COLOR_GREEN;
    if (str_eq(name, "cyan"))        return VGA_COLOR_CYAN;
    if (str_eq(name, "red"))         return VGA_COLOR_RED;
    if (str_eq(name, "magenta"))     return VGA_COLOR_MAGENTA;
    if (str_eq(name, "brown"))       return VGA_COLOR_BROWN;
    if (str_eq(name, "lightgrey"))   return VGA_COLOR_LIGHT_GREY;
    if (str_eq(name, "darkgrey"))    return VGA_COLOR_DARK_GREY;
    if (str_eq(name, "lightblue"))   return VGA_COLOR_LIGHT_BLUE;
    if (str_eq(name, "lightgreen"))  return VGA_COLOR_LIGHT_GREEN;
    if (str_eq(name, "lightcyan"))   return VGA_COLOR_LIGHT_CYAN;
    if (str_eq(name, "lightred"))    return VGA_COLOR_LIGHT_RED;
    if (str_eq(name, "lightmagenta"))return VGA_COLOR_LIGHT_MAGENTA;
    if (str_eq(name, "lightbrown"))  return VGA_COLOR_LIGHT_BROWN;
    if (str_eq(name, "white"))       return VGA_COLOR_WHITE;
    return def;
}

void config_init(void) {
    /* Значения по умолчанию. */
    cfg.fg = VGA_COLOR_WHITE;
    cfg.bg = VGA_COLOR_BLACK;
    cfg.screen_rows = 25;
    cfg.screen_cols = 80;

    const char *default_hostname = "nola";
    int i = 0;
    while (default_hostname[i] && i < (int)sizeof(cfg.hostname) - 1) {
        cfg.hostname[i] = default_hostname[i];
        i++;
    }
    cfg.hostname[i] = '\0';
}

const kernel_config_t *config_get(void) {
    return &cfg;
}

void config_set_hostname(const char *name) {
    if (!name) return;
    while (*name == ' ') name++;
    int i = 0;
    while (name[i] && name[i] != ' ' && i < (int)sizeof(cfg.hostname) - 1) {
        cfg.hostname[i] = name[i];
        i++;
    }
    cfg.hostname[i] = '\0';
}

int config_set_colors(const char *fg_name, const char *bg_name) {
    vga_color_t new_fg = parse_color(fg_name, cfg.fg);
    vga_color_t new_bg = parse_color(bg_name, cfg.bg);
    cfg.fg = new_fg;
    cfg.bg = new_bg;
    vga_set_color(cfg.fg, cfg.bg);
    return 0;
}

void config_set_screen(uint8_t rows, uint8_t cols) {
    if (rows == 0 || cols == 0) return;
    cfg.screen_rows = rows;
    cfg.screen_cols = cols;
}

