#pragma once

#include <stdint.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289u

#define MULTIBOOT2_TAG_TYPE_END 0u
#define MULTIBOOT2_TAG_TYPE_MODULE 3u
#define MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO 4u
#define MULTIBOOT2_TAG_TYPE_MMAP 6u
#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER 8u

#define MULTIBOOT2_MMAP_TYPE_AVAILABLE 1u

struct mb2_info_header {
    uint32_t total_size;
    uint32_t reserved;
};

struct mb2_tag {
    uint32_t type;
    uint32_t size;
};

struct mb2_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char string[0];
};

struct mb2_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
};

struct mb2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
};

struct mb2_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct mb2_mmap_entry entries[0];
};

struct mb2_tag_framebuffer_common {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
};
