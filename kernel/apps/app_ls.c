#include "../console.h"
#include "../fs/vfs.h"

static int print_dirent(const vfs_dirent_t* ent, void* user) {
    (void)user;
    console_print(ent->name);
    if (ent->is_dir) console_putc('/');
    console_putc('\n');
    return 0;
}

int app_ls_main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : ".";

    if (vfs_listdir(path, print_dirent, 0) < 0) {
        console_print("ls: cannot open directory\n");
        return 1;
    }

    return 0;
}
