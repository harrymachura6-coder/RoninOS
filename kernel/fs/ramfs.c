#include "ramfs.h"

#include "../lib/string.h"

typedef struct ramfs_node {
    int used;
    int is_dir;
    int parent;
    int first_child;
    int next_sibling;
    char name[RAMFS_MAX_NAME];
    char data[RAMFS_MAX_CONTENT];
    size_t size;
} ramfs_node_t;

static ramfs_node_t g_nodes[RAMFS_MAX_NODES];

static int s_copy(char* dst, const char* src, size_t max) {
    size_t n = 0;
    if (!src || !src[0]) return -1;
    while (n + 1 < max && src[n]) {
        dst[n] = src[n];
        n++;
    }
    if (src[n]) return -1;
    dst[n] = 0;
    return 0;
}

static int node_alloc(void) {
    size_t i;
    for (i = 0; i < RAMFS_MAX_NODES; i++) {
        if (!g_nodes[i].used) {
            memset(&g_nodes[i], 0, sizeof(g_nodes[i]));
            g_nodes[i].used = 1;
            g_nodes[i].parent = -1;
            g_nodes[i].first_child = -1;
            g_nodes[i].next_sibling = -1;
            return (int)i;
        }
    }
    return -1;
}

static int node_find_child(int dir_idx, const char* name) {
    int child = g_nodes[dir_idx].first_child;
    while (child >= 0) {
        if (strcmp(g_nodes[child].name, name) == 0) return child;
        child = g_nodes[child].next_sibling;
    }
    return -1;
}

static int node_create_child(int dir_idx, const char* name, int is_dir) {
    int idx;
    if (node_find_child(dir_idx, name) >= 0) return -1;
    idx = node_alloc();
    if (idx < 0) return -2;
    if (s_copy(g_nodes[idx].name, name, sizeof(g_nodes[idx].name)) < 0) {
        g_nodes[idx].used = 0;
        return -1;
    }
    g_nodes[idx].is_dir = is_dir;
    g_nodes[idx].parent = dir_idx;
    g_nodes[idx].next_sibling = g_nodes[dir_idx].first_child;
    g_nodes[dir_idx].first_child = idx;
    return idx;
}

static void* fs_get_root(void* ctx) {
    (void)ctx;
    return &g_nodes[0];
}

static int fs_lookup(void* ctx, void* dir, const char* name, void** out_node, int* out_is_dir) {
    int dir_idx;
    int idx;
    (void)ctx;
    if (!dir || !name || !out_node || !out_is_dir) return -1;
    dir_idx = (int)(((ramfs_node_t*)dir) - g_nodes);
    if (dir_idx < 0 || dir_idx >= RAMFS_MAX_NODES || !g_nodes[dir_idx].is_dir) return -1;
    idx = node_find_child(dir_idx, name);
    if (idx < 0) return -1;
    *out_node = &g_nodes[idx];
    *out_is_dir = g_nodes[idx].is_dir;
    return 0;
}

static int fs_create(void* ctx, void* dir, const char* name, int is_dir, void** out_node) {
    int dir_idx;
    int idx;
    (void)ctx;
    if (!dir || !name) return -1;
    dir_idx = (int)(((ramfs_node_t*)dir) - g_nodes);
    if (dir_idx < 0 || dir_idx >= RAMFS_MAX_NODES || !g_nodes[dir_idx].is_dir) return -1;
    idx = node_create_child(dir_idx, name, is_dir);
    if (idx < 0) return idx;
    if (out_node) *out_node = &g_nodes[idx];
    return 0;
}

static int fs_read(void* ctx, void* node, size_t offset, void* buf, size_t n) {
    ramfs_node_t* f = (ramfs_node_t*)node;
    size_t can;
    (void)ctx;
    if (!f || f->is_dir || !buf) return -1;
    if (offset >= f->size) return 0;
    can = f->size - offset;
    if (can > n) can = n;
    memcpy(buf, f->data + offset, can);
    return (int)can;
}

static int fs_write(void* ctx, void* node, size_t offset, const void* buf, size_t n) {
    ramfs_node_t* f = (ramfs_node_t*)node;
    size_t can;
    (void)ctx;
    if (!f || f->is_dir || !buf) return -1;
    if (offset > f->size) offset = f->size;
    can = RAMFS_MAX_CONTENT - offset;
    if (can > n) can = n;
    if (can == 0) return 0;
    memcpy(f->data + offset, buf, can);
    if (offset + can > f->size) f->size = offset + can;
    return (int)can;
}

static int fs_readdir(void* ctx, void* dir, size_t index, const char** out_name, int* out_is_dir, size_t* out_size) {
    int dir_idx;
    int child;
    size_t cur = 0;
    (void)ctx;

    if (!dir || !out_name || !out_is_dir || !out_size) return -1;
    dir_idx = (int)(((ramfs_node_t*)dir) - g_nodes);
    if (dir_idx < 0 || dir_idx >= RAMFS_MAX_NODES || !g_nodes[dir_idx].is_dir) return -1;

    child = g_nodes[dir_idx].first_child;
    while (child >= 0) {
        if (cur == index) {
            *out_name = g_nodes[child].name;
            *out_is_dir = g_nodes[child].is_dir;
            *out_size = g_nodes[child].size;
            return 0;
        }
        cur++;
        child = g_nodes[child].next_sibling;
    }
    return -1;
}

static size_t fs_size(void* ctx, void* node) {
    ramfs_node_t* n = (ramfs_node_t*)node;
    (void)ctx;
    if (!n || n->is_dir) return 0;
    return n->size;
}

static const vfs_fs_ops_t g_ops = {
    fs_get_root,
    fs_lookup,
    fs_create,
    fs_read,
    fs_write,
    fs_readdir,
    fs_size,
};

const vfs_fs_ops_t* ramfs_ops(void) {
    return &g_ops;
}

void ramfs_init(void) {
    memset(g_nodes, 0, sizeof(g_nodes));
    g_nodes[0].used = 1;
    g_nodes[0].is_dir = 1;
    g_nodes[0].parent = 0;
    g_nodes[0].first_child = -1;
    g_nodes[0].next_sibling = -1;
}

int ramfs_touch(const char* name) {
    int idx;
    if (!name || !name[0]) return -1;
    idx = node_find_child(0, name);
    if (idx >= 0) return 0;
    idx = node_create_child(0, name, 0);
    if (idx == -2) return -2;
    return idx < 0 ? -1 : 0;
}

int ramfs_write(const char* name, const char* text, int append) {
    int idx;
    size_t off = 0;
    size_t len;
    if (ramfs_touch(name) < 0) return -1;
    idx = node_find_child(0, name);
    if (idx < 0) return -1;
    if (!text) text = "";
    if (append) off = g_nodes[idx].size;
    else g_nodes[idx].size = 0;
    len = strlen(text);
    if (off + len >= RAMFS_MAX_CONTENT) len = RAMFS_MAX_CONTENT - 1 - off;
    if (len > 0) memcpy(g_nodes[idx].data + off, text, len);
    g_nodes[idx].size = off + len;
    g_nodes[idx].data[g_nodes[idx].size] = 0;
    return 0;
}

const char* ramfs_read(const char* name, size_t* out_size) {
    int idx = node_find_child(0, name);
    if (idx < 0 || g_nodes[idx].is_dir) return 0;
    if (out_size) *out_size = g_nodes[idx].size;
    return g_nodes[idx].data;
}

int ramfs_remove(const char* name) {
    int prev = -1;
    int cur = g_nodes[0].first_child;
    while (cur >= 0) {
        if (strcmp(g_nodes[cur].name, name) == 0) {
            if (prev < 0) g_nodes[0].first_child = g_nodes[cur].next_sibling;
            else g_nodes[prev].next_sibling = g_nodes[cur].next_sibling;
            memset(&g_nodes[cur], 0, sizeof(g_nodes[cur]));
            return 0;
        }
        prev = cur;
        cur = g_nodes[cur].next_sibling;
    }
    return -1;
}

size_t ramfs_count(void) {
    size_t n = 0;
    int cur = g_nodes[0].first_child;
    while (cur >= 0) {
        if (!g_nodes[cur].is_dir) n++;
        cur = g_nodes[cur].next_sibling;
    }
    return n;
}

int ramfs_get_entry(size_t index, ramfs_entry_t* out) {
    size_t curi = 0;
    int cur = g_nodes[0].first_child;
    if (!out) return -1;
    while (cur >= 0) {
        if (!g_nodes[cur].is_dir) {
            if (curi == index) {
                out->name = g_nodes[cur].name;
                out->size = g_nodes[cur].size;
                return 0;
            }
            curi++;
        }
        cur = g_nodes[cur].next_sibling;
    }
    return -1;
}
