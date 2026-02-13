#pragma once

#include "../lib/types.h"

typedef enum {
    BLOCKDEV_TYPE_ATA = 0,
    BLOCKDEV_TYPE_VIRTIO = 1,
    BLOCKDEV_TYPE_MEMDISK = 2,
} blockdev_type_t;

struct blockdev;

typedef int (*blockdev_read_fn)(struct blockdev* dev, unsigned int lba, unsigned int count, void* buf);
typedef int (*blockdev_write_fn)(struct blockdev* dev, unsigned int lba, unsigned int count, const void* buf);

typedef struct blockdev {
    char name[8];
    blockdev_type_t type;
    unsigned int sector_size;
    unsigned int sector_count;
    blockdev_read_fn read;
    blockdev_write_fn write;
    void* ctx;
} blockdev_t;

void block_init(void);
int block_register(const blockdev_t* dev);
size_t block_count(void);
const blockdev_t* block_get(size_t index);
const blockdev_t* block_find(const char* name);
const char* block_type_name(blockdev_type_t type);
