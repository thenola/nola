#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "paging.h"
#include "fs.h"
#include "user.h"
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Простые константы ядра для info/uname/version. */
static const char *KERNEL_NAME    = "nola";
static const char *KERNEL_VERSION = "1.5.0sn";
static const char *KERNEL_BUILD   = __DATE__ " " __TIME__;

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static void cmd_help(void) {
    vga_println("Available commands:");
    vga_println("  help          - show this help");
    vga_println("  clear / cls   - clear screen");
    vga_println("  echo <text>   - print text");
    vga_println("  info          - show info");
    vga_println("  whoami        - show current user");
    vga_println("  su <user>     - switch user (root/user)");
    vga_println("  mem           - show memory info");
    vga_println("  uptime        - show system uptime");
    vga_println("  version       - show kernel version");
    vga_println("  cpuinfo       - display CPU information");
    vga_println("  date          - show build date/time");
    vga_println("  ls [path]     - list directory");
    vga_println("  cd <path>     - change directory");
    vga_println("  pwd           - print current directory");
    vga_println("  cat <file>    - print file contents");
    vga_println("  mkdir <path>  - create directory");
    vga_println("  touch <file>  - create empty file");
    vga_println("  write <f> <t> - write text to file");
    vga_println("  dmesg         - print kernel messages (stub)");
    vga_println("  modules       - list loaded modules (stub)");
    vga_println("  uname -a      - print system information");
    vga_println("  ps            - list processes (stub)");
    vga_println("  kill <pid>    - terminate process (stub)");
    vga_println("  halt          - halt CPU");
    vga_println("  reboot        - reboot machine");
}

static void cmd_info(void) {
    vga_print("Nola 64-bit kernel shell ");
    vga_print(KERNEL_VERSION);
    vga_putc('\n');
}

static void cmd_whoami(void) {
    vga_println(user_get_name());
}

static void cmd_mem(void) {
    uint64_t next = paging_get_next_free();
    vga_print("Next free page address: ");
    vga_print_hex64(next);
    vga_putc('\n');
}

static void cmd_uptime(void) {
    /* Таймера пока нет — просто сообщаем. */
    vga_println("uptime: timer not implemented (no PIT/APIC yet).");
}

static void cmd_version(void) {
    vga_print("version: ");
    vga_print(KERNEL_NAME);
    vga_print(" ");
    vga_print(KERNEL_VERSION);
    vga_putc('\n');
}

static void cmd_cpuinfo(void) {
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0), "c"(0));

    vendor[0]  = (char)(ebx & 0xFF);
    vendor[1]  = (char)((ebx >> 8) & 0xFF);
    vendor[2]  = (char)((ebx >> 16) & 0xFF);
    vendor[3]  = (char)((ebx >> 24) & 0xFF);
    vendor[4]  = (char)(edx & 0xFF);
    vendor[5]  = (char)((edx >> 8) & 0xFF);
    vendor[6]  = (char)((edx >> 16) & 0xFF);
    vendor[7]  = (char)((edx >> 24) & 0xFF);
    vendor[8]  = (char)(ecx & 0xFF);
    vendor[9]  = (char)((ecx >> 8) & 0xFF);
    vendor[10] = (char)((ecx >> 16) & 0xFF);
    vendor[11] = (char)((ecx >> 24) & 0xFF);
    vendor[12] = '\0';

    vga_print("CPU vendor: ");
    vga_println(vendor);
}

static void cmd_date(void) {
    vga_print("build date: ");
    vga_println(KERNEL_BUILD);
}

static void cmd_halt(void) {
    vga_println("Halting CPU...");
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

static void cmd_reboot(void) {
    vga_println("Rebooting...");

    /* Отключаем прерывания на всякий случай. */
    __asm__ volatile("cli");

    /* Классический способ через контроллер клавиатуры 8042. */
    while (inb(0x64) & 0x02) {
        /* ждём пока очистится input buffer */
    }
    outb(0x64, 0xFE); /* команда перезагрузки */

    for (;;) {
        __asm__ volatile("hlt");
    }
}

static void cmd_ps(void) {
    vga_println("  PID   CMD");
    vga_println("    1   [kernel-shell]   (no multitasking implemented)");
}

static void cmd_kill(const char *args) {
    (void)args;
    vga_println("kill: process management not implemented yet.");
}

static void cmd_dmesg(void) {
    vga_println("dmesg: kernel log buffer not implemented yet.");
}

static void cmd_modules(void) {
    vga_println("modules: no loadable modules (monolithic kernel).");
}

static void cmd_uname(const char *args) {
    (void)args;
    /* Простая версия uname -a. */
    vga_print(KERNEL_NAME);
    vga_print(" ");
    vga_print(KERNEL_VERSION);
    vga_print(" ");
    vga_print("x86_64 baremetal");
    vga_putc('\n');
}

static void shell_execute(const char *line) {
    /* Простый парсер: выделяем первое слово и остальное. */
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

    if (cmd[0] == '\0') {
        return;
    }

    if (str_eq(cmd, "help")) {
        cmd_help();
    } else if (str_eq(cmd, "clear") || str_eq(cmd, "cls")) {
        vga_clear();
    } else if (str_eq(cmd, "echo")) {
        vga_print(args);
        vga_putc('\n');
    } else if (str_eq(cmd, "info")) {
        cmd_info();
    } else if (str_eq(cmd, "whoami")) {
        cmd_whoami();
    } else if (str_eq(cmd, "su")) {
        if (user_switch(args) != 0) {
            vga_println("su: unknown user (use 'root' or 'user')");
        } else {
            /* Меняем рабочий каталог в зависимости от пользователя. */
            if (user_is_root()) {
                fs_cd("/");
            } else {
                if (fs_cd("/home/user") != 0) {
                    fs_cd("/");
                }
            }
        }
    } else if (str_eq(cmd, "mem")) {
        cmd_mem();
    } else if (str_eq(cmd, "uptime")) {
        cmd_uptime();
    } else if (str_eq(cmd, "version")) {
        cmd_version();
    } else if (str_eq(cmd, "cpuinfo")) {
        cmd_cpuinfo();
    } else if (str_eq(cmd, "date")) {
        cmd_date();
    } else if (str_eq(cmd, "ls")) {
        if (fs_ls(args) != 0) {
            vga_println("ls: cannot list directory");
        }
    } else if (str_eq(cmd, "cd")) {
        if (fs_cd(args) != 0) {
            vga_println("cd: no such directory");
        }
    } else if (str_eq(cmd, "pwd")) {
        vga_println(fs_pwd());
    } else if (str_eq(cmd, "cat")) {
        char buf[256];
        uint64_t n = 0;
        if (fs_read_file(args, buf, sizeof(buf), &n) != 0) {
            vga_println("cat: cannot read file");
        } else {
            vga_println(buf);
        }
    } else if (str_eq(cmd, "mkdir")) {
        if (fs_mkdir(args) != 0) {
            vga_println("mkdir: cannot create directory");
        }
    } else if (str_eq(cmd, "touch")) {
        if (fs_touch(args) != 0) {
            vga_println("touch: cannot create file");
        }
    } else if (str_eq(cmd, "write")) {
        /* args = "file rest_of_text" */
        char name[64];
        int ni = 0;
        int j = 0;
        while (args[j] == ' ') j++;
        while (args[j] && args[j] != ' ' && ni < (int)sizeof(name) - 1) {
            name[ni++] = args[j++];
        }
        name[ni] = '\0';
        while (args[j] == ' ') j++;
        const char *text = &args[j];
        if (name[0] == '\0' || fs_write_file(name, text) != 0) {
            vga_println("write: usage: write <file> <text>");
        } else {
            vga_println("write: ok");
        }
    } else if (str_eq(cmd, "dmesg")) {
        cmd_dmesg();
    } else if (str_eq(cmd, "modules")) {
        cmd_modules();
    } else if (str_eq(cmd, "uname")) {
        cmd_uname(args);
    } else if (str_eq(cmd, "ps")) {
        cmd_ps();
    } else if (str_eq(cmd, "kill")) {
        cmd_kill(args);
    } else if (str_eq(cmd, "halt")) {
        cmd_halt();
    } else if (str_eq(cmd, "reboot")) {
        cmd_reboot();
    } else if (*cmd) {
        vga_print("Unknown command: ");
        vga_println(cmd);
    }
}

void shell_run(void) {
    char buf[128];
    keyboard_init();

    vga_println("");
    vga_println("Nola shell. Type 'help' for commands.");

    while (1) {
        const char *user = user_get_name();
        const char *path = fs_pwd();

        vga_print(user);
        vga_print("@nola:");
        vga_print(path);
        vga_print(user_is_root() ? "# " : "$ ");

        keyboard_read_line(buf, sizeof(buf));
        shell_execute(buf);
    }
}

