#include "user.h"

static user_id_t current_user = USER_ROOT;

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

void user_init(void) {
    current_user = USER_ROOT;
}

user_id_t user_get_id(void) {
    return current_user;
}

const char *user_get_name(void) {
    return current_user == USER_ROOT ? "root" : "user";
}

int user_is_root(void) {
    return current_user == USER_ROOT;
}

int user_switch(const char *name) {
    if (!name) return -1;
    while (*name == ' ') name++;
    if (*name == '\0') return -1;

    if (str_eq(name, "root")) {
        current_user = USER_ROOT;
        return 0;
    }
    if (str_eq(name, "user")) {
        current_user = USER_NORMAL;
        return 0;
    }
    return -1;
}

