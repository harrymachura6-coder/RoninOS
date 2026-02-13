#include "console.h"
#include "apps/apps.h"
#include "lib/string.h"

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int shell_parse(char* line, char** argv, int max_argv) {
    int argc = 0;
    char* p = line;

    while (*p) {
        while (*p && is_space(*p)) {
            *p = 0;
            p++;
        }

        if (*p == 0 || argc >= max_argv) break;

        if (*p == '"') {
            p++;
            argv[argc++] = p;

            while (*p && *p != '"') p++;
            if (*p == '"') {
                *p = 0;
                p++;
            }
        } else {
            argv[argc++] = p;

            while (*p && !is_space(*p)) p++;
            if (*p) {
                *p = 0;
                p++;
            }
        }
    }

    return argc;
}

static const struct app_entry* find_app(const char* name) {
    size_t i;
    size_t count = 0;
    const struct app_entry* apps = apps_get_table(&count);

    for (i = 0; i < count; i++) {
        if (strcmp(name, apps[i].name) == 0) return &apps[i];
    }

    return 0;
}

static void shell_help(void) {
    size_t i;
    size_t count = 0;
    const struct app_entry* apps = apps_get_table(&count);

    console_print("Available apps:\n");
    for (i = 0; i < count; i++) {
        console_print("  ");
        console_print(apps[i].name);
        console_print(" - ");
        console_print(apps[i].help ? apps[i].help : "(no help)");
        console_putc('\n');
    }

    console_print("\nFAT32 quick guide:\n");
    console_print("  disk                         - list block devices\n");
    console_print("  fat32 select hd0             - select target disk\n");
    console_print("  fat32 format --yes           - format selected disk\n");
    console_print("  fat32 format hd0 --yes       - select + format in one step\n");
    console_print("  fat32 write NAME.EXT \"txt\"  - create file\n");
    console_print("  fat32 edit NAME.EXT \"txt\"   - overwrite/edit file\n");
    console_print("  fat32 cat NAME.EXT           - read file\n");
    console_print("  fat32 rm NAME.EXT            - delete file\n");
    console_print("  fat32 ls                     - list files\n");
}

void shell_on_line(const char* line) {
    char buf[128];
    char* argv[16];
    int argc;
    int i;
    const struct app_entry* app;

    for (i = 0; i < (int)sizeof(buf) - 1 && line[i]; i++) {
        buf[i] = line[i];
    }
    buf[i] = 0;

    argc = shell_parse(buf, argv, 16);
    if (argc == 0) return;

    if (strcmp(argv[0], "help") == 0) {
        shell_help();
        return;
    }

    app = find_app(argv[0]);
    if (!app || !app->main) {
        console_print("Unknown command. Type 'help'.\n");
        return;
    }

    app->main(argc, argv);
}
