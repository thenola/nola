#include "fs.h"
#include "vga.h"

#define FS_MAX_NODES   64
#define FS_NAME_LEN    31

typedef enum {
    FS_NODE_UNUSED = 0,
    FS_NODE_FILE,
    FS_NODE_DIR
} fs_node_type_t;

typedef struct fs_node {
    fs_node_type_t type;
    int parent;                 /* индекс родителя, -1 для корня */
    int first_child;            /* индекс первого ребёнка */
    int next_sibling;           /* односвязный список детей */
    char name[FS_NAME_LEN + 1];
    char *data;
    uint64_t size;
} fs_node_t;

static fs_node_t nodes[FS_MAX_NODES];
static int current_dir = 0;      /* индекс текущего каталога */

static void fs_node_init(int idx, fs_node_type_t type, int parent, const char *name) {
    nodes[idx].type = type;
    nodes[idx].parent = parent;
    nodes[idx].first_child = -1;
    nodes[idx].next_sibling = -1;

    int i = 0;
    while (name && name[i] && i < FS_NAME_LEN) {
        nodes[idx].name[i] = name[i];
        i++;
    }
    nodes[idx].name[i] = '\0';
    nodes[idx].data = 0;
    nodes[idx].size = 0;
}

static int fs_alloc_node(void) {
    for (int i = 0; i < FS_MAX_NODES; ++i) {
        if (nodes[i].type == FS_NODE_UNUSED) {
            return i;
        }
    }
    return -1;
}

static int str_eq_n(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static int fs_find_child(int dir_idx, const char *name) {
    int child = nodes[dir_idx].first_child;
    while (child != -1) {
        if (str_eq_n(nodes[child].name, name)) {
            return child;
        }
        child = nodes[child].next_sibling;
    }
    return -1;
}

static int fs_add_child(int parent_idx, int child_idx) {
    nodes[child_idx].next_sibling = nodes[parent_idx].first_child;
    nodes[parent_idx].first_child = child_idx;
    return 0;
}

/* Разбор пути: поддерживаем /, относительные пути и .. */
static int fs_resolve(const char *path, int base) {
    if (!path || !*path) return base;

    int idx = base;
    if (path[0] == '/') {
        idx = 0; /* корень */
        path++;
    }

    char part[FS_NAME_LEN + 1];
    int pi = 0;

    while (1) {
        char c = *path++;
        if (c == '/' || c == '\0') {
            part[pi] = '\0';
            if (pi > 0) {
                if (str_eq_n(part, ".")) {
                    /* ничего */
                } else if (str_eq_n(part, "..")) {
                    if (nodes[idx].parent != -1)
                        idx = nodes[idx].parent;
                } else {
                    int child = fs_find_child(idx, part);
                    if (child == -1) return -1;
                    idx = child;
                }
            }
            pi = 0;
            if (c == '\0') break;
        } else {
            if (pi < FS_NAME_LEN) {
                part[pi++] = c;
            }
        }
    }

    return idx;
}

void fs_init(void) {
    /* Обнуляем все узлы. */
    for (int i = 0; i < FS_MAX_NODES; ++i) {
        nodes[i].type = FS_NODE_UNUSED;
        nodes[i].parent = -1;
        nodes[i].first_child = -1;
        nodes[i].next_sibling = -1;
        nodes[i].name[0] = '\0';
        nodes[i].data = 0;
        nodes[i].size = 0;
    }

    /* Корень. */
    fs_node_init(0, FS_NODE_DIR, -1, "");
    current_dir = 0;

    /* Примеры каталогов и файлов. */
    int etc = fs_alloc_node();
    if (etc != -1) {
        fs_node_init(etc, FS_NODE_DIR, 0, "etc");
        fs_add_child(0, etc);
    }

    int readme = fs_alloc_node();
    if (readme != -1) {
        fs_node_init(readme, FS_NODE_FILE, 0, "readme.txt");
        static char text[] = "Welcome to Nola shell in-memory FS.\n";
        nodes[readme].data = text;
        nodes[readme].size = sizeof(text) - 1;
        fs_add_child(0, readme);
    }

    /* Домашние каталоги как в Linux. */
    fs_mkdir("/home");
    fs_mkdir("/home/user");
}

int fs_mkdir(const char *path) {
    int parent = current_dir;
    const char *name = path;

    /* Если путь содержит '/', создаём только последний элемент, остальное должно существовать. */
    const char *last_slash = 0;
    for (const char *p = path; *p; ++p) {
        if (*p == '/') last_slash = p;
    }

    if (last_slash) {
        /* временно копируем префикс */
        char tmp[FS_NAME_LEN + 1];
        int len = (int)(last_slash - path);
        if (len > FS_NAME_LEN) len = FS_NAME_LEN;
        for (int i = 0; i < len; ++i) tmp[i] = path[i];
        tmp[len] = '\0';
        parent = fs_resolve(tmp, current_dir);
        if (parent == -1) return -1;
        name = last_slash + 1;
    }

    if (!*name) return -1;
    if (fs_find_child(parent, name) != -1) return -1;

    int idx = fs_alloc_node();
    if (idx == -1) return -1;

    fs_node_init(idx, FS_NODE_DIR, parent, name);
    fs_add_child(parent, idx);
    return 0;
}

int fs_touch(const char *path) {
    int parent = current_dir;
    const char *name = path;

    const char *last_slash = 0;
    for (const char *p = path; *p; ++p) {
        if (*p == '/') last_slash = p;
    }

    if (last_slash) {
        char tmp[FS_NAME_LEN + 1];
        int len = (int)(last_slash - path);
        if (len > FS_NAME_LEN) len = FS_NAME_LEN;
        for (int i = 0; i < len; ++i) tmp[i] = path[i];
        tmp[len] = '\0';
        parent = fs_resolve(tmp, current_dir);
        if (parent == -1) return -1;
        name = last_slash + 1;
    }

    if (!*name) return -1;

    int existing = fs_find_child(parent, name);
    if (existing != -1) {
        if (nodes[existing].type != FS_NODE_FILE) return -1;
        return 0;
    }

    int idx = fs_alloc_node();
    if (idx == -1) return -1;

    fs_node_init(idx, FS_NODE_FILE, parent, name);
    fs_add_child(parent, idx);
    return 0;
}

int fs_write_file(const char *path, const char *data) {
    if (!data) return -1;

    int idx = fs_resolve(path, current_dir);
    if (idx == -1) {
        /* если файла нет — создаём */
        if (fs_touch(path) != 0) return -1;
        idx = fs_resolve(path, current_dir);
        if (idx == -1) return -1;
    }

    if (nodes[idx].type != FS_NODE_FILE) return -1;

    /* Пишем во временный статический буфер (демо, без динамического аллока). */
    static char storage[FS_MAX_NODES][128];
    int slot = idx % FS_MAX_NODES;
    char *dst = storage[slot];

    int i = 0;
    while (data[i] && i < 127) {
        dst[i] = data[i];
        i++;
    }
    dst[i] = '\0';

    nodes[idx].data = dst;
    nodes[idx].size = (uint64_t)i;
    return 0;
}

int fs_read_file(const char *path, char *buf, uint64_t max_len, uint64_t *out_len) {
    int idx = fs_resolve(path, current_dir);
    if (idx == -1 || nodes[idx].type != FS_NODE_FILE) return -1;
    if (!buf || max_len == 0) return -1;

    uint64_t n = nodes[idx].size;
    if (n >= max_len) n = max_len - 1;
    for (uint64_t i = 0; i < n; ++i) {
        buf[i] = nodes[idx].data ? nodes[idx].data[i] : 0;
    }
    buf[n] = '\0';
    if (out_len) *out_len = n;
    return 0;
}

int fs_ls(const char *path) {
    int dir = (path && *path) ? fs_resolve(path, current_dir) : current_dir;
    if (dir == -1 || nodes[dir].type != FS_NODE_DIR) return -1;

    int child = nodes[dir].first_child;
    while (child != -1) {
        if (nodes[child].type == FS_NODE_DIR) {
            vga_print("[DIR] ");
        } else {
            vga_print("FILE ");
        }
        vga_println(nodes[child].name);
        child = nodes[child].next_sibling;
    }
    return 0;
}

int fs_cd(const char *path) {
    if (!path || !*path) return 0;
    int idx = fs_resolve(path, current_dir);
    if (idx == -1 || nodes[idx].type != FS_NODE_DIR) return -1;
    current_dir = idx;
    return 0;
}

const char *fs_pwd(void) {
    /* Строим путь в статический буфер. */
    static char buf[128];
    int pos = 0;
    int idx = current_dir;

    /* Спускаемся вверх к корню, собирая имена в обратном порядке. */
    int depth = 0;
    int stack[FS_MAX_NODES];

    while (idx != -1) {
        stack[depth++] = idx;
        idx = nodes[idx].parent;
    }

    if (depth == 1) {
        buf[0] = '/';
        buf[1] = '\0';
        return buf;
    }

    pos = 0;
    for (int i = depth - 2; i >= 0; --i) { /* пропускаем корень (пустое имя) */
        buf[pos++] = '/';
        const char *name = nodes[stack[i]].name;
        while (*name && pos < (int)sizeof(buf) - 1) {
            buf[pos++] = *name++;
        }
    }
    buf[pos] = '\0';
    return buf;
}

