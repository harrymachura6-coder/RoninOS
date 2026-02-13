#pragma once

#include "../lib/types.h"
#include "../block/blockdev.h"

enum {
    FAT32_NAME83_LEN = 11,
    FAT32_MAX_ROOT_ENTRIES = 128,
};

typedef struct {
    const blockdev_t* bdev;
    unsigned int sector_size;
    unsigned int total_sectors;
} fat32_device_t;

typedef struct {
    fat32_device_t* dev;
    unsigned int sectors_per_cluster;
    unsigned int reserved_sectors;
    unsigned int fat_count;
    unsigned int sectors_per_fat;
    unsigned int root_cluster;
    unsigned int fat_start_lba;
    unsigned int data_start_lba;
    unsigned int total_clusters;
} fat32_fs_t;

typedef struct {
    char name83[FAT32_NAME83_LEN + 1];
    unsigned int size;
} fat32_dirent_t;

int fat32_format(fat32_device_t* dev);
int fat32_mount(fat32_fs_t* fs, fat32_device_t* dev);
int fat32_device_from_blockdev(fat32_device_t* dev, const blockdev_t* bdev);
int fat32_io_read(const fat32_device_t* dev, unsigned int sector_lba, unsigned int count, void* out_buf);
int fat32_io_write(const fat32_device_t* dev, unsigned int sector_lba, unsigned int count, const void* in_buf);

int fat32_list_root(fat32_fs_t* fs, fat32_dirent_t* out, size_t max_out, size_t* out_count);
int fat32_write_file(fat32_fs_t* fs, const char* name, const char* data, size_t len);
int fat32_read_file(fat32_fs_t* fs, const char* name, char* out, size_t out_cap, size_t* out_len);
int fat32_delete_file(fat32_fs_t* fs, const char* name);

const char* fat32_last_error(void);
void fat32_clear_error(void);
