#include "apps.h"

int app_echo_main(int argc, char** argv);
int app_clear_main(int argc, char** argv);
int app_about_main(int argc, char** argv);
int app_meminfo_main(int argc, char** argv);
int app_heap_test_main(int argc, char** argv);
int app_spawn_main(int argc, char** argv);
int app_yield_main(int argc, char** argv);
int app_ps_main(int argc, char** argv);
int app_preempt_main(int argc, char** argv);
int app_fs_main(int argc, char** argv);
int app_fat32_main(int argc, char** argv);
int app_ls_main(int argc, char** argv);
int app_cat_main(int argc, char** argv);
int app_pwd_main(int argc, char** argv);
int app_cd_main(int argc, char** argv);
int app_disk_main(int argc, char** argv);

static const struct app_entry g_apps[] = {
    {"help", "List all kernel apps", 0},
    {"echo", "echo <text> - print text", app_echo_main},
    {"clear", "clear - clear screen", app_clear_main},
    {"about", "about - show kernel info", app_about_main},
    {"meminfo", "meminfo - show heap statistics", app_meminfo_main},
    {"heap_test", "heap_test - run allocator self test", app_heap_test_main},
    {"spawn", "spawn <n> - create worker threads", app_spawn_main},
    {"yield", "yield - switch to next runnable thread", app_yield_main},
    {"ps", "ps - dump scheduler thread table", app_ps_main},
    {"preempt", "preempt on|off - timer scheduling toggle", app_preempt_main},
    {"fs", "fs <cmd> - legacy RAMFS ops (ls/touch/write/append/cat/cp/mv/rm)", app_fs_main},
    {"fat32", "fat32 <cmd> - FAT32 on selected blockdev (select/format/mount/info/ls/...)", app_fat32_main},
    {"ls", "ls [path] - list directory", app_ls_main},
    {"cat", "cat <path> - print file", app_cat_main},
    {"pwd", "pwd - print current directory", app_pwd_main},
    {"cd", "cd [path] - change directory", app_cd_main},
    {"disk", "disk [info/read] - list block devices", app_disk_main},
};

const struct app_entry* apps_get_table(size_t* out_count) {
    if (out_count) {
        *out_count = sizeof(g_apps) / sizeof(g_apps[0]);
    }
    return g_apps;
}
