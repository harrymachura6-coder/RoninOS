#include "blockdev.h"

#include "../lib/string.h"

#define BLOCKDEV_MAX 8

static blockdev_t g_blockdevs[BLOCKDEV_MAX];
static size_t g_blockdev_count;

void block_init(void) {
    g_blockdev_count = 0;
    memset(g_blockdevs, 0, sizeof(g_blockdevs));
}

int block_register(const blockdev_t* dev) {
    if (!dev || !dev->read || dev->sector_size == 0 || dev->sector_count == 0) return -1;
    if (g_blockdev_count >= BLOCKDEV_MAX) return -1;

    g_blockdevs[g_blockdev_count] = *dev;
    g_blockdev_count++;
    return 0;
}

size_t block_count(void) {
    return g_blockdev_count;
}

const blockdev_t* block_get(size_t index) {
    if (index >= g_blockdev_count) return 0;
    return &g_blockdevs[index];
}

const blockdev_t* block_find(const char* name) {
    size_t i;
    if (!name) return 0;

    for (i = 0; i < g_blockdev_count; i++) {
        if (strcmp(g_blockdevs[i].name, name) == 0) {
            return &g_blockdevs[i];
        }
    }
    return 0;
}

const char* block_type_name(blockdev_type_t type) {
    if (type == BLOCKDEV_TYPE_ATA) return "ATA";
    if (type == BLOCKDEV_TYPE_VIRTIO) return "VIRTIO";
    if (type == BLOCKDEV_TYPE_MEMDISK) return "MEMDISK";
    return "UNKNOWN";
}
