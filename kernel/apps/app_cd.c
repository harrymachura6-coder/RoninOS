#include "../console.h"
#include "../fs/vfs.h"

int app_cd_main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "/";
    if (vfs_chdir(path) < 0) {
        console_print("cd: no such directory\n");
        return 1;
    }
    return 0;
}
