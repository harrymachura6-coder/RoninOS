#include "../block/blockdev.h"
#include "../console.h"
#include "../fs/fat32.h"
#include "../lib/string.h"

static const blockdev_t* g_selected;
static int g_mounted;

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

static void print_capacity(unsigned int sector_size, unsigned int sector_count) {
    unsigned int whole_mib;
    unsigned int frac;

    if (sector_size == 0) {
        console_print("0.00 MiB");
        return;
    }

    whole_mib = (sector_count / 2048u) * (sector_size / 512u);
    frac = ((sector_count % 2048u) * 100u) / 2048u;

    print_u32(whole_mib);
    console_putc('.');
    console_putc((char)('0' + (frac / 10u)));
    console_putc((char)('0' + (frac % 10u)));
    console_print(" MiB");
}

static void usage(void) {
    console_print("fat32 select <disk>\n");
    console_print("fat32 format [disk] --yes\n");
    console_print("fat32 mount\n");
    console_print("fat32 info\n");
    console_print("fat32 ls\n");
    console_print("fat32 write <NAME.EXT> <text>\n");
    console_print("fat32 edit <NAME.EXT> <text>\n");
    console_print("fat32 cat <NAME.EXT>\n");
    console_print("fat32 rm <NAME.EXT>\n");
    console_print("fat32 log\n");
}

static int build_selected_dev(fat32_device_t* dev) {
    if (!g_selected) {
        console_print("no device selected\n");
        return -1;
    }
    if (fat32_device_from_blockdev(dev, g_selected) != 0) {
        console_print("unable to use selected device\n");
        return -1;
    }
    return 0;
}

static int ensure_mounted(fat32_fs_t* fs) {
    fat32_device_t dev;

    if (build_selected_dev(&dev) != 0) return -1;
    if (fat32_mount(fs, &dev) != 0) {
        g_mounted = 0;
        console_print("mount failed\n");
        console_print("reason: ");
        console_print(fat32_last_error());
        console_putc('\n');
        return -1;
    }
    g_mounted = 1;
    return 0;
}

static int cmd_select(const char* name) {
    const blockdev_t* found = block_find(name);

    if (!found) {
        console_print("device not found\n");
        return 1;
    }

    g_selected = found;
    g_mounted = 0;
    console_print("selected: ");
    console_print(g_selected->name);
    console_putc('\n');
    return 0;
}

static int cmd_format(int argc, char** argv) {
    const char* disk_name = 0;
    int has_yes = 0;
    int i;
    fat32_device_t dev;

    for (i = 2; i < argc; i++) {
        if (streq(argv[i], "--yes")) {
            has_yes = 1;
        } else if (!disk_name) {
            disk_name = argv[i];
        }
    }

    if (disk_name) {
        if (cmd_select(disk_name) != 0) return 1;
    }

    if (!g_selected) {
        console_print("no device selected\n");
        return 1;
    }
    if (!has_yes) {
        console_print("refusing to format without confirmation. use: fat32 format ");
        console_print(g_selected->name);
        console_print(" --yes\n");
        return 1;
    }
    if (!g_selected->write) {
        console_print("device is read-only / write not supported\n");
        return 1;
    }

    if (build_selected_dev(&dev) != 0) return 1;
    if (dev.sector_size != 512) {
        console_print("unsupported sector size\n");
        return 1;
    }

    if (fat32_format(&dev) != 0) {
        console_print("format failed\n");
        console_print("reason: ");
        console_print(fat32_last_error());
        console_putc('\n');
        return 1;
    }

    g_mounted = 0;
    console_print("fat32 formatted on ");
    console_print(g_selected->name);
    console_putc('\n');
    return 0;
}

static int cmd_info(void) {
    fat32_device_t dev;
    fat32_fs_t fs;

    if (!g_selected) {
        console_print("no device selected\n");
        return 1;
    }

    if (build_selected_dev(&dev) != 0) return 1;

    console_print("selected device: ");
    console_print(g_selected->name);
    console_putc('\n');
    console_print("type: ");
    console_print(block_type_name(g_selected->type));
    console_putc('\n');
    console_print("sector size: ");
    print_u32(g_selected->sector_size);
    console_print("\nsector count: ");
    print_u32(g_selected->sector_count);
    console_print("\ncapacity: ");
    print_capacity(g_selected->sector_size, g_selected->sector_count);
    console_putc('\n');

    if (fat32_mount(&fs, &dev) != 0) {
        g_mounted = 0;
        console_print("formatted: no\n");
        console_print("mounted: no\n");
        return 0;
    }

    g_mounted = 1;
    console_print("formatted: yes\n");
    console_print("mounted: yes\n");
    console_print("bytes/sector: ");
    print_u32(dev.sector_size);
    console_print("\nsectors/cluster: ");
    print_u32(fs.sectors_per_cluster);
    console_print("\nreserved sectors: ");
    print_u32(fs.reserved_sectors);
    console_print("\nfat count: ");
    print_u32(fs.fat_count);
    console_print("\nfat sectors: ");
    print_u32(fs.sectors_per_fat);
    console_print("\nroot cluster: ");
    print_u32(fs.root_cluster);
    console_putc('\n');
    return 0;
}

int app_fat32_main(int argc, char** argv) {
    fat32_fs_t fs;

    if (argc < 2) {
        usage();
        return 1;
    }

    if (streq(argv[1], "select")) {
        if (argc < 3) {
            console_print("usage: fat32 select <disk>\n");
            return 1;
        }
        return cmd_select(argv[2]);
    }

    if (streq(argv[1], "format")) {
        return cmd_format(argc, argv);
    }

    if (streq(argv[1], "mount")) {
        if (ensure_mounted(&fs) != 0) return 1;
        console_print("mount ok\n");
        return 0;
    }

    if (streq(argv[1], "info")) {
        return cmd_info();
    }

    if (streq(argv[1], "ls")) {
        fat32_dirent_t entries[16];
        size_t i;
        size_t count = 0;

        if (ensure_mounted(&fs) != 0) return 1;
        if (fat32_list_root(&fs, entries, 16, &count) != 0) {
            console_print("ls failed\n");
            console_print("reason: ");
            console_print(fat32_last_error());
            console_putc('\n');
            return 1;
        }

        if (count == 0) {
            console_print("root empty\n");
            return 0;
        }

        for (i = 0; i < count; i++) {
            console_print(entries[i].name83);
            console_print(" (");
            print_u32(entries[i].size);
            console_print(" B)\n");
        }
        return 0;
    }

    if (streq(argv[1], "write") || streq(argv[1], "edit")) {
        if (argc < 4) {
            console_print("usage: fat32 write|edit <NAME.EXT> <text>\n");
            return 1;
        }

        if (ensure_mounted(&fs) != 0) return 1;
        if (fat32_write_file(&fs, argv[2], argv[3], strlen(argv[3])) != 0) {
            console_print("write failed (name must be 8.3, data <= 512 bytes)\n");
            console_print("reason: ");
            console_print(fat32_last_error());
            console_putc('\n');
            return 1;
        }
        return 0;
    }

    if (streq(argv[1], "rm")) {
        if (argc < 3) {
            console_print("usage: fat32 rm <NAME.EXT>\n");
            return 1;
        }

        if (ensure_mounted(&fs) != 0) return 1;
        if (fat32_delete_file(&fs, argv[2]) != 0) {
            console_print("delete failed\n");
            console_print("reason: ");
            console_print(fat32_last_error());
            console_putc('\n');
            return 1;
        }
        return 0;
    }

    if (streq(argv[1], "cat")) {
        char buf[513];
        size_t out_len;

        if (argc < 3) {
            console_print("usage: fat32 cat <NAME.EXT>\n");
            return 1;
        }

        if (ensure_mounted(&fs) != 0) return 1;
        if (fat32_read_file(&fs, argv[2], buf, sizeof(buf), &out_len) != 0) {
            console_print("read failed\n");
            console_print("reason: ");
            console_print(fat32_last_error());
            console_putc('\n');
            return 1;
        }
        (void)out_len;
        console_print(buf);
        console_putc('\n');
        return 0;
    }

    if (streq(argv[1], "log")) {
        console_print("fat32 last error: ");
        console_print(fat32_last_error());
        console_putc('\n');
        return 0;
    }

    usage();
    return 1;
}
