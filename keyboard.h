#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
char keyboard_getchar(void);
void keyboard_read_line(char *buf, uint64_t max_len);

#endif /* KEYBOARD_H */

