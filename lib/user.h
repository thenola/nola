#ifndef USER_H
#define USER_H

typedef enum {
    USER_ROOT = 0,
    USER_NORMAL = 1,
} user_id_t;

void user_init(void);
user_id_t user_get_id(void);
const char *user_get_name(void);
int user_is_root(void);
int user_switch(const char *name); /* 0 = ok, -1 = fail */

#endif /* USER_H */

