#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "paging.h"
#include "heap.h"
#include "fs.h"
#include "config.h"
#include <stdint.h>

static const char *KERNEL_NAME    = "nola";
static const char *KERNEL_VERSION = "0226b";

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static void cmd_help(void) {
    vga_println("help         - show commands");
    vga_println("clear / cls  - clear screen");
    vga_println("echo <text>  - print text");
    vga_println("ls [path]    - list directory");
    vga_println("cd <path>    - change directory");
    vga_println("pwd          - print current directory");
    vga_println("cat <file>   - print file contents");
    vga_println("mkdir <path> - create directory");
    vga_println("touch <file> - create empty file");
    vga_println("write <f> <t> - write text to file");
    vga_println("mem          - memory info");
    vga_println("version      - kernel version");
    vga_println("halt         - halt CPU");
    vga_println("reboot       - reboot");
}

static void cmd_mem(void) {
    uint64_t next = paging_get_next_free();
    vga_print("next free: ");
    vga_print_hex64(next);
    vga_putc('\n');
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void cmd_halt(void) {
    vga_println("Halting...");
    for (;;) __asm__ volatile("cli; hlt");
}

static void cmd_reboot(void) {
    vga_println("Rebooting...");
    __asm__ volatile("cli");
    while (inb(0x64) & 0x02) {}
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile("hlt");
}

static void shell_execute(const char *line) {
    char cmd[16];
    const char *args = 0;

    int i = 0;
    while (line[i] == ' ') i++;
    int ci = 0;
    while (line[i] && line[i] != ' ' && ci < (int)sizeof(cmd) - 1) {
        cmd[ci++] = line[i++];
    }
    cmd[ci] = '\0';
    while (line[i] == ' ') i++;
    args = &line[i];

    if (cmd[0] == '\0') return;

    if (str_eq(cmd, "help")) {
        cmd_help();
    } else if (str_eq(cmd, "clear") || str_eq(cmd, "cls")) {
        vga_clear();
    } else if (str_eq(cmd, "echo")) {
        vga_print(args);
        vga_putc('\n');
    } else if (str_eq(cmd, "ls")) {
        if (fs_ls(args) != 0) vga_println("ls: error");
    } else if (str_eq(cmd, "cd")) {
        if (fs_cd(args) != 0) vga_println("cd: no such directory");
    } else if (str_eq(cmd, "pwd")) {
        vga_println(fs_pwd());
    } else if (str_eq(cmd, "cat")) {
        char buf[256];
        uint64_t n = 0;
        if (fs_read_file(args, buf, sizeof(buf), &n) != 0) {
            vga_println("cat: cannot read");
        } else {
            vga_println(buf);
        }
    } else if (str_eq(cmd, "mkdir")) {
        if (fs_mkdir(args) != 0) vga_println("mkdir: error");
    } else if (str_eq(cmd, "touch")) {
        if (fs_touch(args) != 0) vga_println("touch: error");
    } else if (str_eq(cmd, "write")) {
        char name[64];
        int ni = 0, j = 0;
        while (args[j] == ' ') j++;
        while (args[j] && args[j] != ' ' && ni < (int)sizeof(name) - 1) {
            name[ni++] = args[j++];
        }
        name[ni] = '\0';
        while (args[j] == ' ') j++;
        if (name[0] == '\0' || fs_write_file(name, &args[j]) != 0) {
            vga_println("write: usage write <file> <text>");
        } else {
            vga_println("ok");
        }
    } else if (str_eq(cmd, "mem")) {
        cmd_mem();
    } else if (str_eq(cmd, "version")) {
        vga_print(KERNEL_NAME);
        vga_print(" ");
        vga_print(KERNEL_VERSION);
        vga_putc('\n');
    } else if (str_eq(cmd, "halt")) {
        cmd_halt();
    } else if (str_eq(cmd, "reboot")) {
        cmd_reboot();
    } else {
        vga_print("unknown: ");
        vga_println(cmd);
    }
}

void shell_run(void) {
    char buf[128];
    keyboard_init();

    vga_println("");
    vga_println("Nola shell. Type 'help'.");

    while (1) {
        vga_print("nola:");
        vga_print(fs_pwd());
        vga_print("> ");

        keyboard_read_line(buf, sizeof(buf));
        shell_execute(buf);
    }
}
