#include "../console.h"
#include "../fs/ramfs.h"

static void print_u32(unsigned int n) {
    char buf[11];
    int i = 0;

    if (n == 0) {
        console_putc('0');
        return;
    }

    while (n > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }

    while (i > 0) {
        i--;
        console_putc(buf[i]);
    }
}

static int streq(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void print_usage(void) {
    console_print("legacy: fs arbeitet direkt auf RAMFS-root\n");
    console_print("fs ls\n");
    console_print("fs touch <name>\n");
    console_print("fs write <name> <text>\n");
    console_print("fs append <name> <text>\n");
    console_print("fs cat <name>\n");
    console_print("fs cp <src> <dst>\n");
    console_print("fs mv <src> <dst>\n");
    console_print("fs rm <name>\n");
}

static int cmd_ls(void) {
    size_t i;
    size_t count = ramfs_count();

    if (count == 0) {
        console_print("ramfs empty\n");
        return 0;
    }

    for (i = 0; i < count; i++) {
        ramfs_entry_t entry;
        if (ramfs_get_entry(i, &entry) == 0) {
            console_print(entry.name);
            console_print(" (");
            print_u32((unsigned int)entry.size);
            console_print(" B)\n");
        }
    }

    return 0;
}

int app_fs_main(int argc, char** argv) {
    int rc;

    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (streq(argv[1], "ls")) {
        return cmd_ls();
    }

    if (streq(argv[1], "touch")) {
        if (argc < 3) {
            console_print("usage: fs touch <name>\n");
            return 1;
        }
        rc = ramfs_touch(argv[2]);
        if (rc == -1) {
            console_print("invalid file name\n");
            return 1;
        }
        if (rc == -2) {
            console_print("ramfs is full\n");
            return 1;
        }
        return 0;
    }

    if (streq(argv[1], "write") || streq(argv[1], "append")) {
        int append = streq(argv[1], "append");

        if (argc < 4) {
            console_print("usage: fs write|append <name> <text>\n");
            return 1;
        }

        rc = ramfs_write(argv[2], argv[3], append);
        if (rc == -1) {
            console_print("invalid file name\n");
            return 1;
        }
        if (rc == -2) {
            console_print("ramfs is full\n");
            return 1;
        }
        if (rc == 1) {
            console_print("warning: text truncated\n");
        }
        return 0;
    }

    if (streq(argv[1], "cat")) {
        const char* data;

        if (argc < 3) {
            console_print("usage: fs cat <name>\n");
            return 1;
        }

        data = ramfs_read(argv[2], 0);
        if (!data) {
            console_print("file not found\n");
            return 1;
        }
        console_print(data);
        console_putc('\n');
        return 0;
    }

    if (streq(argv[1], "rm")) {
        if (argc < 3) {
            console_print("usage: fs rm <name>\n");
            return 1;
        }

        if (ramfs_remove(argv[2]) < 0) {
            console_print("file not found\n");
            return 1;
        }
        return 0;
    }

    if (streq(argv[1], "cp") || streq(argv[1], "mv")) {
        const char* data;
        int move = streq(argv[1], "mv");

        if (argc < 4) {
            console_print("usage: fs cp|mv <src> <dst>\n");
            return 1;
        }

        data = ramfs_read(argv[2], 0);
        if (!data) {
            console_print("source file not found\n");
            return 1;
        }

        rc = ramfs_write(argv[3], data, 0);
        if (rc == -1) {
            console_print("invalid destination file name\n");
            return 1;
        }
        if (rc == -2) {
            console_print("ramfs is full\n");
            return 1;
        }
        if (rc == 1) {
            console_print("warning: text truncated\n");
        }

        if (move) {
            if (ramfs_remove(argv[2]) < 0) {
                console_print("warning: source cleanup failed\n");
            }
        }

        return 0;
    }

    print_usage();
    return 1;
}
