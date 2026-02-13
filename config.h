#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "vga.h"

typedef struct kernel_config {
    char       hostname[32];
    vga_color_t fg;
    vga_color_t bg;
    uint8_t    screen_rows;
    uint8_t    screen_cols;
} kernel_config_t;

void config_init(void);
const kernel_config_t *config_get(void);
void config_set_hostname(const char *name);
int  config_set_colors(const char *fg_name, const char *bg_name);
void config_set_screen(uint8_t rows, uint8_t cols);

#endif /* CONFIG_H */

