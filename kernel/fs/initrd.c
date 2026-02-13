#include "initrd.h"

#include "vfs.h"
#include "../console.h"
#include "../lib/string.h"
#include "../mem/multiboot2.h"

#define INITRD_MAX_FILES 32
#define INITRD_MAX_NAME 32

typedef struct {
    int used;
    const uint8_t* data;
    size_t size;
    char name[INITRD_MAX_NAME];
} initrd_file_t;

typedef struct {
    initrd_file_t files[INITRD_MAX_FILES];
    size_t count;
    int root_sentinel;
} initrd_ctx_t;

static initrd_ctx_t g_initrd;

static uint32_t tag_end(const struct mb2_tag* tag) {
    return (uint32_t)((uintptr_t)tag + ((tag->size + 7u) & ~7u));
}

static void append_u32(char* out, uint32_t n) {
    char rev[11];
    int i = 0;
    int j;
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return;
    }
    while (n > 0 && i < 10) {
        rev[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }
    for (j = 0; j < i; j++) out[j] = rev[i - 1 - j];
    out[i] = 0;
}

static void basename_from_cmdline(const char* cmdline, char* out, size_t out_sz, uint32_t idx) {
    const char* start = cmdline;
    const char* p = cmdline;
    const char* last = cmdline;
    size_t n = 0;

    if (!cmdline || !cmdline[0]) {
        const char* pre = "module";
        memcpy(out, pre, 6);
        append_u32(out + 6, idx);
        return;
    }

    while (*p && *p != ' ') p++;
    while (start < p) {
        if (*start == '/') last = start + 1;
        start++;
    }

    while (last[n] && last[n] != ' ' && n + 1 < out_sz) {
        out[n] = last[n];
        n++;
    }

    if (n == 0) {
        const char* pre = "module";
        memcpy(out, pre, 6);
        append_u32(out + 6, idx);
        return;
    }
    out[n] = 0;
}

static void* initrd_get_root(void* ctx) {
    initrd_ctx_t* fs = (initrd_ctx_t*)ctx;
    return &fs->root_sentinel;
}

static int initrd_lookup(void* ctx, void* dir, const char* name, void** out_node, int* out_is_dir) {
    initrd_ctx_t* fs = (initrd_ctx_t*)ctx;
    size_t i;

    if (!fs || !dir || !name || !out_node || !out_is_dir) return -1;
    if (dir != &fs->root_sentinel) return -1;

    for (i = 0; i < fs->count; i++) {
        if (fs->files[i].used && strcmp(fs->files[i].name, name) == 0) {
            *out_node = &fs->files[i];
            *out_is_dir = 0;
            return 0;
        }
    }
    return -1;
}

static int initrd_create(void* ctx, void* dir, const char* name, int is_dir, void** out_node) {
    (void)ctx;
    (void)dir;
    (void)name;
    (void)is_dir;
    (void)out_node;
    return -1;
}

static int initrd_read(void* ctx, void* node, size_t offset, void* buf, size_t n) {
    initrd_file_t* file = (initrd_file_t*)node;
    size_t can;
    (void)ctx;

    if (!file || !buf) return -1;
    if (offset >= file->size) return 0;

    can = file->size - offset;
    if (can > n) can = n;
    memcpy(buf, file->data + offset, can);
    return (int)can;
}

static int initrd_write(void* ctx, void* node, size_t offset, const void* buf, size_t n) {
    (void)ctx;
    (void)node;
    (void)offset;
    (void)buf;
    (void)n;
    return -1;
}

static int initrd_readdir(void* ctx, void* dir, size_t index, const char** out_name, int* out_is_dir, size_t* out_size) {
    initrd_ctx_t* fs = (initrd_ctx_t*)ctx;
    size_t i;
    size_t current = 0;

    if (!fs || !dir || !out_name || !out_is_dir || !out_size) return -1;
    if (dir != &fs->root_sentinel) return -1;

    for (i = 0; i < fs->count; i++) {
        if (!fs->files[i].used) continue;
        if (current == index) {
            *out_name = fs->files[i].name;
            *out_is_dir = 0;
            *out_size = fs->files[i].size;
            return 0;
        }
        current++;
    }
    return -1;
}

static size_t initrd_size(void* ctx, void* node) {
    initrd_file_t* file = (initrd_file_t*)node;
    (void)ctx;
    if (!file) return 0;
    return file->size;
}

static const vfs_fs_ops_t g_initrd_ops = {
    initrd_get_root,
    initrd_lookup,
    initrd_create,
    initrd_read,
    initrd_write,
    initrd_readdir,
    initrd_size,
};

void initrd_mount_from_multiboot(uint32_t mb_magic, uint32_t mb_info_addr) {
    const struct mb2_info_header* info;
    const struct mb2_tag* tag;
    const struct mb2_tag* end_tag;
    uint32_t mod_idx = 0;

    memset(&g_initrd, 0, sizeof(g_initrd));
    if (vfs_mount("/initrd", &g_initrd_ops, &g_initrd) < 0) {
        console_print("initrd: mount failed\n");
        return;
    }

    if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC || mb_info_addr == 0u) {
        console_print("initrd: no multiboot2 modules\n");
        return;
    }

    info = (const struct mb2_info_header*)(uintptr_t)mb_info_addr;
    tag = (const struct mb2_tag*)((uintptr_t)info + 8u);
    end_tag = (const struct mb2_tag*)((uintptr_t)info + info->total_size - 8u);

    while ((uintptr_t)tag < (uintptr_t)end_tag && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE) {
            const struct mb2_tag_module* mod = (const struct mb2_tag_module*)tag;

            if (g_initrd.count < INITRD_MAX_FILES) {
                initrd_file_t* file = &g_initrd.files[g_initrd.count++];
                file->used = 1;
                file->data = (const uint8_t*)(uintptr_t)mod->mod_start;
                file->size = (size_t)(mod->mod_end - mod->mod_start);
                basename_from_cmdline(mod->string, file->name, sizeof(file->name), mod_idx);
            }
            mod_idx++;
        }
        tag = (const struct mb2_tag*)(uintptr_t)tag_end(tag);
    }

    if (g_initrd.count == 0) console_print("initrd: mounted empty\n");
    else console_print("initrd: mounted modules\n");
}
