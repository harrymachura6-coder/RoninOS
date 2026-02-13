#include "../console.h"
#include "../fs/vfs.h"

int app_pwd_main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_print(vfs_getcwd());
    console_putc('\n');
    return 0;
}
