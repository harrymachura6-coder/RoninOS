#pragma once

#include "../lib/types.h"

#define VFS_MAX_PATH 128
#define VFS_MAX_NAME 24

#define VFS_O_RDONLY 0x01
#define VFS_O_WRONLY 0x02
#define VFS_O_RDWR 0x03
#define VFS_O_CREATE 0x10

typedef struct vfs_fs_ops {
    void* (*get_root)(void* ctx);
    int (*lookup)(void* ctx, void* dir, const char* name, void** out_node, int* out_is_dir);
    int (*create)(void* ctx, void* dir, const char* name, int is_dir, void** out_node);
    int (*read)(void* ctx, void* node, size_t offset, void* buf, size_t n);
    int (*write)(void* ctx, void* node, size_t offset, const void* buf, size_t n);
    int (*readdir)(void* ctx, void* dir, size_t index, const char** out_name, int* out_is_dir, size_t* out_size);
    size_t (*size)(void* ctx, void* node);
} vfs_fs_ops_t;

typedef struct vfs_dirent {
    const char* name;
    int is_dir;
    size_t size;
} vfs_dirent_t;

typedef struct vfs_stat {
    int is_dir;
    size_t size;
} vfs_stat_t;

typedef int (*vfs_listdir_cb_t)(const vfs_dirent_t* ent, void* user);

void vfs_init(void);
int vfs_mount(const char* mountpoint, const vfs_fs_ops_t* ops, void* ctx);

int vfs_open(const char* path, int flags);
int vfs_read(int fd, void* buf, size_t n);
int vfs_write(int fd, const void* buf, size_t n);
int vfs_close(int fd);

int vfs_mkdir(const char* path);
int vfs_create(const char* path);

int vfs_listdir(const char* path, vfs_listdir_cb_t callback, void* user);
int vfs_stat(const char* path, vfs_stat_t* out_stat);

int vfs_chdir(const char* path);
const char* vfs_getcwd(void);
