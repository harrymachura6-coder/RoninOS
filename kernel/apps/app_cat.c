#include "../console.h"
#include "../fs/vfs.h"

int app_cat_main(int argc, char** argv) {
    int fd;
    char buf[64];
    int n;

    if (argc < 2) {
        console_print("usage: cat <path>\n");
        return 1;
    }

    fd = vfs_open(argv[1], VFS_O_RDONLY);
    if (fd < 0) {
        console_print("cat: file not found\n");
        return 1;
    }

    while ((n = vfs_read(fd, buf, sizeof(buf))) > 0) {
        int i;
        for (i = 0; i < n; i++) console_putc(buf[i]);
    }
    vfs_close(fd);
    console_putc('\n');
    return 0;
}
