#include "vfs.h"

#include "../lib/string.h"

#define VFS_MAX_MOUNTS 4
#define VFS_MAX_FD 32

typedef struct {
    int used;
    char mountpoint[VFS_MAX_PATH];
    const vfs_fs_ops_t* ops;
    void* ctx;
} vfs_mount_t;

typedef struct {
    int used;
    int flags;
    const vfs_fs_ops_t* ops;
    void* ctx;
    void* node;
    size_t pos;
} vfs_fd_t;

static vfs_mount_t g_mounts[VFS_MAX_MOUNTS];
static vfs_fd_t g_fds[VFS_MAX_FD];
static char g_cwd[VFS_MAX_PATH];

static size_t s_len(const char* s, size_t max) {
    size_t i = 0;
    while (i < max && s[i]) i++;
    return i;
}

static int copy_str(char* dst, const char* src, size_t max) {
    size_t n = s_len(src, max - 1);
    if (src[n] != 0) return -1;
    memcpy(dst, src, n);
    dst[n] = 0;
    return 0;
}

static int normalize_path(const char* in, char* out) {
    char temp[VFS_MAX_PATH];
    char segs[16][VFS_MAX_NAME];
    int top = 0;
    size_t i = 0;
    size_t out_len;

    if (!in || !out) return -1;

    if (in[0] == '/') {
        if (copy_str(temp, in, sizeof(temp)) < 0) return -1;
    } else {
        size_t cwd_len = s_len(g_cwd, sizeof(g_cwd));
        if (cwd_len + 1 >= sizeof(temp)) return -1;
        memcpy(temp, g_cwd, cwd_len);
        if (cwd_len > 1) {
            temp[cwd_len++] = '/';
        }
        if (copy_str(temp + cwd_len, in, sizeof(temp) - cwd_len) < 0) return -1;
    }

    while (temp[i]) {
        size_t j = 0;
        char seg[VFS_MAX_NAME];

        while (temp[i] == '/') i++;
        if (!temp[i]) break;

        while (temp[i] && temp[i] != '/' && j + 1 < sizeof(seg)) {
            seg[j++] = temp[i++];
        }
        seg[j] = 0;

        while (temp[i] && temp[i] != '/') i++;

        if (strcmp(seg, ".") == 0) {
            continue;
        }
        if (strcmp(seg, "..") == 0) {
            if (top > 0) top--;
            continue;
        }

        if (top >= 16) return -1;
        if (copy_str(segs[top], seg, sizeof(segs[top])) < 0) return -1;
        top++;
    }

    out[0] = '/';
    out[1] = 0;
    out_len = 1;

    for (i = 0; i < (size_t)top; i++) {
        size_t n = s_len(segs[i], sizeof(segs[i]));
        if (out_len + n + 1 >= VFS_MAX_PATH) return -1;
        if (out_len > 1) out[out_len++] = '/';
        memcpy(out + out_len, segs[i], n);
        out_len += n;
        out[out_len] = 0;
    }

    return 0;
}

static int starts_with(const char* a, const char* b) {
    size_t n = strlen(b);
    if (strncmp(a, b, n) != 0) return 0;
    return a[n] == 0 || a[n] == '/';
}

static const vfs_mount_t* find_mount(const char* path, const char** subpath) {
    size_t i;
    const vfs_mount_t* best = 0;
    size_t best_len = 0;

    for (i = 0; i < VFS_MAX_MOUNTS; i++) {
        size_t mlen;
        if (!g_mounts[i].used) continue;
        if (!starts_with(path, g_mounts[i].mountpoint)) continue;
        mlen = strlen(g_mounts[i].mountpoint);
        if (mlen >= best_len) {
            best = &g_mounts[i];
            best_len = mlen;
        }
    }

    if (!best) return 0;

    if (subpath) {
        *subpath = path + best_len;
        if (**subpath == '/') (*subpath)++;
    }
    return best;
}

static int resolve_on_mount(const vfs_mount_t* m, const char* subpath, void** out_node, int* out_is_dir) {
    void* cur;
    int is_dir = 1;
    size_t i = 0;

    if (!m || !m->ops || !m->ops->get_root || !m->ops->lookup) return -1;

    cur = m->ops->get_root(m->ctx);
    if (!subpath || subpath[0] == 0) {
        if (out_node) *out_node = cur;
        if (out_is_dir) *out_is_dir = 1;
        return 0;
    }

    while (subpath[i]) {
        char seg[VFS_MAX_NAME];
        size_t j = 0;
        void* next = 0;

        if (!is_dir) return -1;

        while (subpath[i] == '/') i++;
        if (!subpath[i]) break;

        while (subpath[i] && subpath[i] != '/' && j + 1 < sizeof(seg)) {
            seg[j++] = subpath[i++];
        }
        seg[j] = 0;
        while (subpath[i] && subpath[i] != '/') i++;

        if (m->ops->lookup(m->ctx, cur, seg, &next, &is_dir) < 0) return -1;
        cur = next;
    }

    if (out_node) *out_node = cur;
    if (out_is_dir) *out_is_dir = is_dir;
    return 0;
}

static int resolve_path(const char* path, const vfs_mount_t** out_mount, void** out_node, int* out_is_dir) {
    char norm[VFS_MAX_PATH];
    const char* subpath;
    const vfs_mount_t* m;

    if (normalize_path(path, norm) < 0) return -1;
    m = find_mount(norm, &subpath);
    if (!m) return -1;
    if (resolve_on_mount(m, subpath, out_node, out_is_dir) < 0) return -1;

    if (out_mount) *out_mount = m;
    return 0;
}

static int resolve_parent(const char* path, const vfs_mount_t** out_mount, void** out_parent, char* out_leaf) {
    char norm[VFS_MAX_PATH];
    const char* cut;
    char parent[VFS_MAX_PATH];

    if (normalize_path(path, norm) < 0) return -1;
    if (strcmp(norm, "/") == 0) return -1;

    cut = norm + strlen(norm);
    while (cut > norm && *cut != '/') cut--;

    if (*cut != '/') return -1;
    if (copy_str(out_leaf, cut + 1, VFS_MAX_NAME) < 0) return -1;

    if (cut == norm) {
        parent[0] = '/';
        parent[1] = 0;
    } else {
        size_t n = (size_t)(cut - norm);
        if (n + 1 >= sizeof(parent)) return -1;
        memcpy(parent, norm, n);
        parent[n] = 0;
    }

    return resolve_path(parent, out_mount, out_parent, 0);
}

void vfs_init(void) {
    memset(g_mounts, 0, sizeof(g_mounts));
    memset(g_fds, 0, sizeof(g_fds));
    g_cwd[0] = '/';
    g_cwd[1] = 0;
}

int vfs_mount(const char* mountpoint, const vfs_fs_ops_t* ops, void* ctx) {
    size_t i;
    char norm[VFS_MAX_PATH];

    if (!ops) return -1;
    if (normalize_path(mountpoint, norm) < 0) return -1;

    for (i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (!g_mounts[i].used) {
            g_mounts[i].used = 1;
            copy_str(g_mounts[i].mountpoint, norm, sizeof(g_mounts[i].mountpoint));
            g_mounts[i].ops = ops;
            g_mounts[i].ctx = ctx;
            return 0;
        }
    }
    return -1;
}

int vfs_open(const char* path, int flags) {
    size_t i;
    const vfs_mount_t* m;
    void* node = 0;
    int is_dir = 0;

    if (resolve_path(path, &m, &node, &is_dir) < 0 || is_dir) {
        if ((flags & VFS_O_CREATE) == 0) return -1;
        if (vfs_create(path) < 0) return -1;
        if (resolve_path(path, &m, &node, &is_dir) < 0 || is_dir) return -1;
    }

    for (i = 0; i < VFS_MAX_FD; i++) {
        if (!g_fds[i].used) {
            g_fds[i].used = 1;
            g_fds[i].flags = flags;
            g_fds[i].ops = m->ops;
            g_fds[i].ctx = m->ctx;
            g_fds[i].node = node;
            g_fds[i].pos = 0;
            return (int)i;
        }
    }
    return -1;
}

int vfs_read(int fd, void* buf, size_t n) {
    int rc;
    if (fd < 0 || fd >= VFS_MAX_FD || !g_fds[fd].used || !g_fds[fd].ops->read) return -1;
    if ((g_fds[fd].flags & VFS_O_WRONLY) == VFS_O_WRONLY) return -1;
    rc = g_fds[fd].ops->read(g_fds[fd].ctx, g_fds[fd].node, g_fds[fd].pos, buf, n);
    if (rc > 0) g_fds[fd].pos += (size_t)rc;
    return rc;
}

int vfs_write(int fd, const void* buf, size_t n) {
    int rc;
    if (fd < 0 || fd >= VFS_MAX_FD || !g_fds[fd].used || !g_fds[fd].ops->write) return -1;
    if ((g_fds[fd].flags & VFS_O_WRONLY) == 0 && (g_fds[fd].flags & VFS_O_RDWR) != VFS_O_RDWR) return -1;
    rc = g_fds[fd].ops->write(g_fds[fd].ctx, g_fds[fd].node, g_fds[fd].pos, buf, n);
    if (rc > 0) g_fds[fd].pos += (size_t)rc;
    return rc;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_FD || !g_fds[fd].used) return -1;
    memset(&g_fds[fd], 0, sizeof(g_fds[fd]));
    return 0;
}

int vfs_mkdir(const char* path) {
    const vfs_mount_t* m;
    void* parent;
    char leaf[VFS_MAX_NAME];
    if (resolve_parent(path, &m, &parent, leaf) < 0 || !m->ops->create) return -1;
    return m->ops->create(m->ctx, parent, leaf, 1, 0);
}

int vfs_create(const char* path) {
    const vfs_mount_t* m;
    void* parent;
    char leaf[VFS_MAX_NAME];
    if (resolve_parent(path, &m, &parent, leaf) < 0 || !m->ops->create) return -1;
    return m->ops->create(m->ctx, parent, leaf, 0, 0);
}

int vfs_listdir(const char* path, vfs_listdir_cb_t callback, void* user) {
    const vfs_mount_t* m;
    void* node;
    int is_dir;
    size_t index = 0;

    if (!callback) return -1;
    if (resolve_path(path, &m, &node, &is_dir) < 0 || !is_dir || !m->ops->readdir) return -1;

    for (;;) {
        const char* name;
        int ent_is_dir;
        size_t ent_size;
        vfs_dirent_t ent;

        if (m->ops->readdir(m->ctx, node, index, &name, &ent_is_dir, &ent_size) < 0) break;

        ent.name = name;
        ent.is_dir = ent_is_dir;
        ent.size = ent_size;

        if (callback(&ent, user) != 0) break;
        index++;
    }

    return 0;
}

int vfs_stat(const char* path, vfs_stat_t* out_stat) {
    const vfs_mount_t* m;
    void* node;
    int is_dir;

    if (!out_stat) return -1;
    if (resolve_path(path, &m, &node, &is_dir) < 0) return -1;

    out_stat->is_dir = is_dir;
    out_stat->size = 0;
    if (!is_dir && m->ops->size) {
        out_stat->size = m->ops->size(m->ctx, node);
    }
    return 0;
}

int vfs_chdir(const char* path) {
    char norm[VFS_MAX_PATH];
    int is_dir = 0;
    if (normalize_path(path, norm) < 0) return -1;
    if (resolve_path(norm, 0, 0, &is_dir) < 0 || !is_dir) return -1;
    copy_str(g_cwd, norm, sizeof(g_cwd));
    return 0;
}

const char* vfs_getcwd(void) {
    return g_cwd;
}
