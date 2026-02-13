#include "../block/blockdev.h"
#include "../console.h"
#include "../lib/string.h"

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

static void print_size_gib(unsigned int sectors) {
    unsigned int whole = sectors / 2097152u;
    unsigned int rem = sectors % 2097152u;
    unsigned int frac = (rem * 100u) / 2097152u;

    print_u32(whole);
    console_putc('.');
    console_putc((char)('0' + (frac / 10u)));
    console_putc((char)('0' + (frac % 10u)));
    console_print(" GiB");
}

static int streq(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int parse_u32(const char* s, unsigned int* out) {
    unsigned int value = 0;

    if (!s || !*s) return -1;
    while (*s) {
        if (*s < '0' || *s > '9') return -1;
        value = value * 10u + (unsigned int)(*s - '0');
        s++;
    }

    *out = value;
    return 0;
}

static void usage(void) {
    console_print("disk\n");
    console_print("disk info <name>\n");
    console_print("disk read <name> <lba> <count>\n");
}

static void print_dev_line(const blockdev_t* dev) {
    console_print(dev->name);
    console_print("  ");
    console_print(block_type_name(dev->type));
    console_print("  ");
    print_u32(dev->sector_size);
    console_print("B/sector  ");
    print_u32(dev->sector_count);
    console_print(" sectors  ");
    print_size_gib(dev->sector_count);
    console_putc('\n');
}

static void print_hex_byte(unsigned char b) {
    static const char* hexdig = "0123456789ABCDEF";
    console_putc(hexdig[(b >> 4) & 0x0F]);
    console_putc(hexdig[b & 0x0F]);
}

static int cmd_list(void) {
    size_t i;
    size_t cnt = block_count();

    if (cnt == 0) {
        console_print("no block devices\n");
        return 0;
    }

    for (i = 0; i < cnt; i++) {
        const blockdev_t* dev = block_get(i);
        if (dev) print_dev_line(dev);
    }
    return 0;
}

static int cmd_info(const char* name) {
    const blockdev_t* dev = block_find(name);

    if (!dev) {
        console_print("disk not found\n");
        return 1;
    }

    print_dev_line(dev);
    return 0;
}

static int cmd_read(const char* name, const char* lba_s, const char* count_s) {
    const blockdev_t* dev = block_find(name);
    unsigned int lba;
    unsigned int count;
    unsigned char buf[512];
    unsigned int max_print = 256;
    unsigned int i;

    if (!dev) {
        console_print("disk not found\n");
        return 1;
    }
    if (parse_u32(lba_s, &lba) != 0 || parse_u32(count_s, &count) != 0) {
        console_print("invalid lba/count\n");
        return 1;
    }
    if (count == 0 || count > 1) {
        console_print("count must be 1 for now\n");
        return 1;
    }
    if (dev->sector_size > sizeof(buf)) {
        console_print("sector size unsupported\n");
        return 1;
    }

    if (dev->read((blockdev_t*)dev, lba, count, buf) != 0) {
        console_print("read failed\n");
        return 1;
    }

    if (dev->sector_size < max_print) max_print = dev->sector_size;
    for (i = 0; i < max_print; i++) {
        if ((i % 16u) == 0) {
            print_u32(i);
            console_print(": ");
        }
        print_hex_byte(buf[i]);
        console_putc(' ');
        if ((i % 16u) == 15u) console_putc('\n');
    }

    return 0;
}

int app_disk_main(int argc, char** argv) {
    if (argc == 1) {
        return cmd_list();
    }

    if (streq(argv[1], "info")) {
        if (argc < 3) {
            usage();
            return 1;
        }
        return cmd_info(argv[2]);
    }

    if (streq(argv[1], "read")) {
        if (argc < 5) {
            usage();
            return 1;
        }
        return cmd_read(argv[2], argv[3], argv[4]);
    }

    usage();
    return 1;
}
