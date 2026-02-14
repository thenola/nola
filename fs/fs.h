#ifndef FS_H
#define FS_H

#include <stdint.h>

void fs_init(void);

int  fs_mkdir(const char *path);
int  fs_touch(const char *path);
int  fs_write_file(const char *path, const char *data);
int  fs_read_file(const char *path, char *buf, uint64_t max_len, uint64_t *out_len);
int  fs_ls(const char *path);
int  fs_cd(const char *path);
const char *fs_pwd(void);

#endif /* FS_H */

