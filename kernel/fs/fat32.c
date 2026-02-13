#include "fat32.h"

#include "../lib/string.h"

#define FAT32_ATTR_ARCHIVE 0x20
#define FAT32_EOC 0x0FFFFFFFu

static char g_fat32_last_error[96] = "ok";

static void fat32_set_error(const char* msg) {
    size_t i;
    if (!msg) msg = "unknown";
    for (i = 0; i < sizeof(g_fat32_last_error) - 1 && msg[i]; i++) {
        g_fat32_last_error[i] = msg[i];
    }
    g_fat32_last_error[i] = 0;
}

const char* fat32_last_error(void) {
    return g_fat32_last_error;
}

void fat32_clear_error(void) {
    fat32_set_error("ok");
}

int fat32_device_from_blockdev(fat32_device_t* dev, const blockdev_t* bdev) {
    if (!dev || !bdev || !bdev->read) { fat32_set_error("device or read callback missing"); return -1; }
    dev->bdev = bdev;
    dev->sector_size = bdev->sector_size;
    dev->total_sectors = bdev->sector_count;
    return 0;
}

int fat32_io_read(const fat32_device_t* dev, unsigned int sector_lba, unsigned int count, void* out_buf) {
    if (!dev || !dev->bdev || !dev->bdev->read || !out_buf || count == 0) { fat32_set_error("invalid read call"); return -1; }
    if (dev->bdev->read((blockdev_t*)dev->bdev, sector_lba, count, out_buf) != 0) {
        fat32_set_error("block read failed");
        return -1;
    }
    return 0;
}

int fat32_io_write(const fat32_device_t* dev, unsigned int sector_lba, unsigned int count, const void* in_buf) {
    if (!dev || !dev->bdev || !dev->bdev->write || !in_buf || count == 0) { fat32_set_error("invalid write call"); return -1; }
    if (dev->bdev->write((blockdev_t*)dev->bdev, sector_lba, count, in_buf) != 0) {
        fat32_set_error("block write failed");
        return -1;
    }
    return 0;
}

static unsigned short rd16(const unsigned char* p) {
    return (unsigned short)p[0] | ((unsigned short)p[1] << 8);
}

static unsigned int rd32(const unsigned char* p) {
    return (unsigned int)p[0] |
           ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) |
           ((unsigned int)p[3] << 24);
}

static void wr16(unsigned char* p, unsigned short v) {
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)((v >> 8) & 0xFFu);
}

static void wr32(unsigned char* p, unsigned int v) {
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)((v >> 8) & 0xFFu);
    p[2] = (unsigned char)((v >> 16) & 0xFFu);
    p[3] = (unsigned char)((v >> 24) & 0xFFu);
}

static unsigned int cluster_to_lba(const fat32_fs_t* fs, unsigned int cluster) {
    return fs->data_start_lba + (cluster - 2u) * fs->sectors_per_cluster;
}

static int make_name83(const char* name, char out11[11]) {
    size_t i;
    size_t pos = 0;
    size_t ext_pos = 8;

    for (i = 0; i < 11; i++) out11[i] = ' ';
    if (!name || !name[0]) return -1;

    for (i = 0; name[i]; i++) {
        char c = name[i];
        if (c == '.') {
            if (pos == 0 || pos > 8 || ext_pos != 8) return -1;
            pos = ext_pos;
            continue;
        }

        if (c >= 'a' && c <= 'z') c = (char)(c - ('a' - 'A'));
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return -1;
        }

        if (pos < 8) {
            out11[pos++] = c;
        } else if (pos >= 8 && pos < 11) {
            out11[pos++] = c;
        } else {
            return -1;
        }
    }

    return 0;
}

static unsigned int fat_get(fat32_fs_t* fs, unsigned int cluster) {
    unsigned char sec[512];
    unsigned int off = cluster * 4u;
    unsigned int lba = fs->fat_start_lba + (off / 512u);
    unsigned int in_sec = off % 512u;

    if (fat32_io_read(fs->dev, lba, 1, sec) != 0) return FAT32_EOC;
    return rd32(sec + in_sec) & 0x0FFFFFFFu;
}

static int fat_set(fat32_fs_t* fs, unsigned int cluster, unsigned int val) {
    unsigned char sec[512];
    unsigned int off = cluster * 4u;
    unsigned int lba = fs->fat_start_lba + (off / 512u);
    unsigned int in_sec = off % 512u;

    unsigned int f;

    if (fat32_io_read(fs->dev, lba, 1, sec) != 0) return -1;
    wr32(sec + in_sec, val);
    for (f = 0; f < fs->fat_count; f++) {
        unsigned int fat_lba = lba + f * fs->sectors_per_fat;
        if (fat32_io_write(fs->dev, fat_lba, 1, sec) != 0) return -1;
    }
    return 0;
}

static int alloc_cluster(fat32_fs_t* fs, unsigned int* out_cluster) {
    unsigned int c;
    for (c = 2; c < fs->total_clusters + 2u; c++) {
        if (fat_get(fs, c) == 0) {
            if (fat_set(fs, c, FAT32_EOC) != 0) return -1;
            *out_cluster = c;
            return 0;
        }
    }
    return -1;
}

static int find_root_entry(fat32_fs_t* fs, const char name83[11], unsigned int* out_lba, unsigned int* out_off) {
    unsigned char sec[512];
    unsigned int lba = cluster_to_lba(fs, fs->root_cluster);
    unsigned int i;

    if (fat32_io_read(fs->dev, lba, 1, sec) != 0) return -1;
    for (i = 0; i < 512; i += 32) {
        if (sec[i] == 0x00) break;
        if (sec[i] == 0xE5) continue;
        if ((sec[i + 11] & 0x0Fu) == 0x0Fu) continue;
        if (strncmp((const char*)(sec + i), name83, 11) == 0) {
            if (out_lba) *out_lba = lba;
            if (out_off) *out_off = i;
            return 0;
        }
    }
    return -1;
}

int fat32_format(fat32_device_t* dev) {
    fat32_clear_error();
    unsigned char sec[512];
    unsigned int reserved = 32;
    unsigned int fat_count = 2;
    unsigned int sectors_per_cluster = 1;
    unsigned int sectors_per_fat = 1;
    unsigned int data_sectors;
    unsigned int total_clusters;
    unsigned int fat_start;
    unsigned int data_start;
    unsigned int iter;
    unsigned int bytes_per_fat;
    unsigned int free_clusters;

    if (!dev || !dev->bdev || !dev->bdev->read || !dev->bdev->write) {
        fat32_set_error("device missing read/write support");
        return -1;
    }
    if (dev->sector_size != 512) {
        fat32_set_error("sector size must be 512 bytes");
        return -1;
    }
    if (dev->total_sectors < 256) {
        fat32_set_error("disk too small");
        return -1;
    }

    for (iter = 0; iter < 8; iter++) {
        if (dev->total_sectors <= reserved + fat_count * sectors_per_fat + 1) {
            fat32_set_error("disk layout cannot fit FAT32 metadata");
            return -1;
        }
        data_sectors = dev->total_sectors - reserved - fat_count * sectors_per_fat;
        total_clusters = data_sectors / sectors_per_cluster;
        bytes_per_fat = (total_clusters + 2u) * 4u;
        bytes_per_fat = (bytes_per_fat + 511u) & ~511u;
        if ((bytes_per_fat / 512u) == sectors_per_fat) break;
        sectors_per_fat = bytes_per_fat / 512u;
    }

    if (dev->total_sectors <= reserved + fat_count * sectors_per_fat + 1) {
        fat32_set_error("disk layout cannot fit FAT32 data area");
        return -1;
    }

    data_sectors = dev->total_sectors - reserved - fat_count * sectors_per_fat;
    total_clusters = data_sectors / sectors_per_cluster;
    if (total_clusters < 2) { fat32_set_error("not enough clusters"); return -1; }
    free_clusters = total_clusters - 1u;
    fat_start = reserved;
    data_start = reserved + fat_count * sectors_per_fat;

    memset(sec, 0, sizeof(sec));
    sec[0] = 0xEB;
    sec[1] = 0x58;
    sec[2] = 0x90;
    memcpy(sec + 3, "RONINOS ", 8);
    wr16(sec + 11, 512);
    sec[13] = (unsigned char)sectors_per_cluster;
    wr16(sec + 14, (unsigned short)reserved);
    sec[16] = (unsigned char)fat_count;
    wr16(sec + 17, 0);
    wr16(sec + 19, 0);
    sec[21] = 0xF8;
    wr16(sec + 22, 0);
    wr16(sec + 24, 63);
    wr16(sec + 26, 255);
    wr32(sec + 28, 0);
    wr32(sec + 32, dev->total_sectors);
    wr32(sec + 36, sectors_per_fat);
    wr16(sec + 40, 0);
    wr16(sec + 42, 0);
    wr32(sec + 44, 2);
    wr16(sec + 48, 1);
    wr16(sec + 50, 6);
    sec[64] = 0x80;
    sec[66] = 0x29;
    wr32(sec + 67, 0x12345678u);
    memcpy(sec + 71, "RONINOS    ", 11);
    memcpy(sec + 82, "FAT32   ", 8);
    sec[510] = 0x55;
    sec[511] = 0xAA;
    if (fat32_io_write(dev, 0, 1, sec) != 0) { fat32_set_error("write boot sector failed"); return -1; }

    memset(sec, 0, sizeof(sec));
    wr32(sec + 0, 0x41615252u);
    wr32(sec + 484, 0x61417272u);
    wr32(sec + 488, free_clusters);
    wr32(sec + 492, 3);
    sec[510] = 0x55;
    sec[511] = 0xAA;
    if (fat32_io_write(dev, 1, 1, sec) != 0) { fat32_set_error("write fsinfo failed"); return -1; }

    memset(sec, 0, sizeof(sec));
    sec[510] = 0x55;
    sec[511] = 0xAA;
    if (fat32_io_write(dev, 2, 1, sec) != 0) { fat32_set_error("write boot backup area failed"); return -1; }

    if (fat32_io_read(dev, 0, 1, sec) != 0) { fat32_set_error("cannot read boot sector"); return -1; }
    if (fat32_io_write(dev, 6, 1, sec) != 0) { fat32_set_error("write backup boot sector failed"); return -1; }

    if (fat32_io_read(dev, 1, 1, sec) != 0) { fat32_set_error("read fsinfo for backup failed"); return -1; }
    if (fat32_io_write(dev, 7, 1, sec) != 0) { fat32_set_error("write backup fsinfo failed"); return -1; }

    memset(sec, 0, sizeof(sec));
    wr32(sec + 0, 0x0FFFFFF8u);
    wr32(sec + 4, FAT32_EOC);
    wr32(sec + 8, FAT32_EOC);
    if (fat32_io_write(dev, fat_start, 1, sec) != 0) { fat32_set_error("write FAT #0 failed"); return -1; }
    if (fat32_io_write(dev, fat_start + sectors_per_fat, 1, sec) != 0) { fat32_set_error("write FAT #1 failed"); return -1; }

    memset(sec, 0, sizeof(sec));
    if (fat32_io_write(dev, data_start, 1, sec) != 0) { fat32_set_error("write root directory cluster failed"); return -1; }
    fat32_set_error("ok");
    return 0;
}

int fat32_mount(fat32_fs_t* fs, fat32_device_t* dev) {
    fat32_clear_error();
    unsigned char sec[512];
    unsigned int total_sectors;

    if (!fs || !dev) { fat32_set_error("mount arguments invalid"); return -1; }
    if (dev->sector_size != 512) { fat32_set_error("unsupported sector size"); return -1; }
    if (fat32_io_read(dev, 0, 1, sec) != 0) { fat32_set_error("cannot read boot sector"); return -1; }

    if (sec[510] != 0x55 || sec[511] != 0xAA) { fat32_set_error("boot signature missing"); return -1; }
    if (rd16(sec + 11) != 512) { fat32_set_error("invalid bytes/sector in BPB"); return -1; }
    if (rd16(sec + 17) != 0) { fat32_set_error("not a FAT32 BPB"); return -1; }

    fs->dev = dev;
    fs->sectors_per_cluster = sec[13];
    fs->reserved_sectors = rd16(sec + 14);
    fs->fat_count = sec[16];
    fs->sectors_per_fat = rd32(sec + 36);
    fs->root_cluster = rd32(sec + 44);
    fs->fat_start_lba = fs->reserved_sectors;
    fs->data_start_lba = fs->reserved_sectors + fs->fat_count * fs->sectors_per_fat;

    total_sectors = rd32(sec + 32);
    if (!total_sectors) total_sectors = rd16(sec + 19);
    if (total_sectors <= fs->data_start_lba) { fat32_set_error("invalid layout: no data area"); return -1; }

    fs->total_clusters = (total_sectors - fs->data_start_lba) / fs->sectors_per_cluster;
    if (fs->total_clusters == 0) { fat32_set_error("invalid cluster count"); return -1; }
    fat32_set_error("ok");
    return 0;
}

int fat32_list_root(fat32_fs_t* fs, fat32_dirent_t* out, size_t max_out, size_t* out_count) {
    unsigned char sec[512];
    unsigned int lba;
    unsigned int i;
    size_t found = 0;

    if (!fs || !out || max_out == 0) { fat32_set_error("invalid ls arguments"); return -1; }

    lba = cluster_to_lba(fs, fs->root_cluster);
    if (fat32_io_read(fs->dev, lba, 1, sec) != 0) { fat32_set_error("read root directory failed"); return -1; }

    for (i = 0; i < 512 && found < max_out; i += 32) {
        if (sec[i] == 0x00) break;
        if (sec[i] == 0xE5) continue;
        if ((sec[i + 11] & 0x0Fu) == 0x0Fu) continue;

        memcpy(out[found].name83, sec + i, 11);
        out[found].name83[11] = 0;
        out[found].size = rd32(sec + i + 28);
        found++;
    }

    if (out_count) *out_count = found;
    return 0;
}

int fat32_write_file(fat32_fs_t* fs, const char* name, const char* data, size_t len) {
    unsigned char sec[512];
    unsigned int lba;
    unsigned int off;
    unsigned int cluster;
    unsigned int data_lba;
    unsigned short hi;
    unsigned short lo;
    char name83[11];

    if (!fs || !data) { fat32_set_error("invalid write arguments"); return -1; }
    if (len > 512u * fs->sectors_per_cluster) { fat32_set_error("file too large for current implementation"); return -1; }
    if (make_name83(name, name83) != 0) { fat32_set_error("invalid 8.3 filename"); return -1; }

    lba = cluster_to_lba(fs, fs->root_cluster);
    if (fat32_io_read(fs->dev, lba, 1, sec) != 0) { fat32_set_error("read root entry sector failed"); return -1; }

    if (find_root_entry(fs, name83, &lba, &off) == 0) {
        hi = rd16(sec + off + 20);
        lo = rd16(sec + off + 26);
        cluster = ((unsigned int)hi << 16) | lo;
        if (cluster >= 2) {
            if (fat_set(fs, cluster, 0) != 0) { fat32_set_error("free old cluster failed"); return -1; }
        }
    } else {
        for (off = 0; off < 512; off += 32) {
            if (sec[off] == 0x00 || sec[off] == 0xE5) break;
        }
        if (off >= 512) { fat32_set_error("root directory is full"); return -1; }
    }

    if (alloc_cluster(fs, &cluster) != 0) { fat32_set_error("no free cluster available"); return -1; }

    memset(sec + off, 0, 32);
    memcpy(sec + off, name83, 11);
    sec[off + 11] = FAT32_ATTR_ARCHIVE;
    wr16(sec + off + 20, (unsigned short)((cluster >> 16) & 0xFFFFu));
    wr16(sec + off + 26, (unsigned short)(cluster & 0xFFFFu));
    wr32(sec + off + 28, (unsigned int)len);
    if (fat32_io_write(fs->dev, lba, 1, sec) != 0) { fat32_set_error("write directory entry failed"); return -1; }

    memset(sec, 0, sizeof(sec));
    memcpy(sec, data, len);
    data_lba = cluster_to_lba(fs, cluster);
    if (fat32_io_write(fs->dev, data_lba, 1, sec) != 0) { fat32_set_error("write file data failed"); return -1; }
    fat32_set_error("ok");
    return 0;
}

int fat32_read_file(fat32_fs_t* fs, const char* name, char* out, size_t out_cap, size_t* out_len) {
    unsigned char sec[512];
    unsigned int lba;
    unsigned int off;
    unsigned int cluster;
    unsigned int data_lba;
    unsigned int len;
    unsigned short hi;
    unsigned short lo;
    char name83[11];

    if (!fs || !out || out_cap == 0) { fat32_set_error("invalid read arguments"); return -1; }
    if (make_name83(name, name83) != 0) { fat32_set_error("invalid 8.3 filename"); return -1; }

    if (find_root_entry(fs, name83, &lba, &off) != 0) { fat32_set_error("file not found"); return -1; }
    if (fat32_io_read(fs->dev, lba, 1, sec) != 0) { fat32_set_error("read directory entry failed"); return -1; }

    len = rd32(sec + off + 28);
    hi = rd16(sec + off + 20);
    lo = rd16(sec + off + 26);
    cluster = ((unsigned int)hi << 16) | lo;
    if (cluster < 2) { fat32_set_error("invalid file cluster"); return -1; }

    if (len + 1 > out_cap) { fat32_set_error("output buffer too small"); return -1; }
    data_lba = cluster_to_lba(fs, cluster);
    if (fat32_io_read(fs->dev, data_lba, 1, sec) != 0) { fat32_set_error("read file data failed"); return -1; }

    memcpy(out, sec, len);
    out[len] = 0;
    if (out_len) *out_len = len;
    fat32_set_error("ok");
    return 0;
}

int fat32_delete_file(fat32_fs_t* fs, const char* name) {
    unsigned char sec[512];
    unsigned int lba;
    unsigned int off;
    unsigned int cluster;
    unsigned short hi;
    unsigned short lo;
    char name83[11];

    if (!fs) { fat32_set_error("invalid delete arguments"); return -1; }
    if (make_name83(name, name83) != 0) { fat32_set_error("invalid 8.3 filename"); return -1; }
    if (find_root_entry(fs, name83, &lba, &off) != 0) { fat32_set_error("file not found"); return -1; }
    if (fat32_io_read(fs->dev, lba, 1, sec) != 0) { fat32_set_error("read directory entry failed"); return -1; }

    hi = rd16(sec + off + 20);
    lo = rd16(sec + off + 26);
    cluster = ((unsigned int)hi << 16) | lo;
    if (cluster >= 2) {
        if (fat_set(fs, cluster, 0) != 0) { fat32_set_error("free cluster failed"); return -1; }
    }

    sec[off] = 0xE5;
    if (fat32_io_write(fs->dev, lba, 1, sec) != 0) { fat32_set_error("write delete marker failed"); return -1; }
    fat32_set_error("ok");
    return 0;
}
