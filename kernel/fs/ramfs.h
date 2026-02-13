#pragma once

#include "../lib/types.h"
#include "vfs.h"

enum {
    RAMFS_MAX_NODES = 96,
    RAMFS_MAX_NAME = 24,
    RAMFS_MAX_CONTENT = 512,
};

typedef struct {
    const char* name;
    size_t size;
} ramfs_entry_t;

void ramfs_init(void);
const vfs_fs_ops_t* ramfs_ops(void);

int ramfs_touch(const char* name);
int ramfs_write(const char* name, const char* text, int append);
const char* ramfs_read(const char* name, size_t* out_size);
int ramfs_remove(const char* name);

size_t ramfs_count(void);
int ramfs_get_entry(size_t index, ramfs_entry_t* out);
