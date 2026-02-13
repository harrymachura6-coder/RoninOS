#include "ata_pio.h"

#include "../block/blockdev.h"
#include "../console.h"
#include "../lib/string.h"
#include "../ports.h"

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_PRIMARY_IO 0x1F0
#define ATA_SECONDARY_IO 0x170
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_CTRL 0x376

#define ATA_SEL_MASTER 0xA0
#define ATA_SEL_SLAVE 0xB0

typedef struct {
    unsigned short io;
    unsigned short ctrl;
    unsigned char slave;
} ata_ctx_t;

static ata_ctx_t g_ata_ctx[4];
static blockdev_t g_ata_devs[4];

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
}

static unsigned short inw16(unsigned short port) {
    unsigned short ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void io_delay_400ns(unsigned short ctrl) {
    inb(ctrl);
    inb(ctrl);
    inb(ctrl);
    inb(ctrl);
}

static int ata_poll_ready(unsigned short io, unsigned short ctrl) {
    unsigned int timeout = 1000000;

    io_delay_400ns(ctrl);
    while (timeout--) {
        unsigned char status = inb(io + 7);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) return -1;
            if (status & ATA_SR_DRQ) return 0;
            if (status & ATA_SR_DRDY) return 0;
        }
    }
    return -1;
}

static int ata_identify(ata_ctx_t* ctx, unsigned int* out_sectors) {
    unsigned short identify[256];
    unsigned char status;
    unsigned int i;

    outb(ctx->io + 6, (unsigned char)((ctx->slave ? ATA_SEL_SLAVE : ATA_SEL_MASTER) | 0x40));
    io_delay_400ns(ctx->ctrl);

    outb(ctx->io + 2, 0);
    outb(ctx->io + 3, 0);
    outb(ctx->io + 4, 0);
    outb(ctx->io + 5, 0);
    outb(ctx->io + 7, ATA_CMD_IDENTIFY);

    status = inb(ctx->io + 7);
    if (status == 0) return -1;

    while ((status & ATA_SR_BSY) != 0) {
        status = inb(ctx->io + 7);
    }

    if (inb(ctx->io + 4) != 0 || inb(ctx->io + 5) != 0) {
        return -1;
    }

    while ((status & ATA_SR_DRQ) == 0) {
        if (status & ATA_SR_ERR) return -1;
        status = inb(ctx->io + 7);
    }

    for (i = 0; i < 256; i++) {
        identify[i] = inw16(ctx->io);
    }

    *out_sectors = ((unsigned int)identify[61] << 16) | identify[60];
    return *out_sectors > 0 ? 0 : -1;
}

static int ata_pio_read(struct blockdev* dev, unsigned int lba, unsigned int count, void* buf) {
    ata_ctx_t* ctx;
    unsigned int i;
    unsigned int s;
    unsigned short* out;

    if (!dev || !buf || count == 0) return -1;
    if (dev->sector_size != 512) return -1;
    if (lba + count > dev->sector_count) return -1;
    if (count > 255) return -1;

    ctx = (ata_ctx_t*)dev->ctx;
    if (!ctx) return -1;

    outb(ctx->ctrl, 0x00);
    outb(ctx->io + 6, (unsigned char)((ctx->slave ? ATA_SEL_SLAVE : ATA_SEL_MASTER) | ((lba >> 24) & 0x0Fu)));
    outb(ctx->io + 2, (unsigned char)count);
    outb(ctx->io + 3, (unsigned char)(lba & 0xFFu));
    outb(ctx->io + 4, (unsigned char)((lba >> 8) & 0xFFu));
    outb(ctx->io + 5, (unsigned char)((lba >> 16) & 0xFFu));
    outb(ctx->io + 7, ATA_CMD_READ_PIO);

    out = (unsigned short*)buf;
    for (s = 0; s < count; s++) {
        if (ata_poll_ready(ctx->io, ctx->ctrl) != 0) return -1;

        for (i = 0; i < 256; i++) {
            out[s * 256u + i] = inw16(ctx->io);
        }
        io_delay_400ns(ctx->ctrl);
    }

    return 0;
}

static int ata_pio_write(struct blockdev* dev, unsigned int lba, unsigned int count, const void* buf) {
    ata_ctx_t* ctx;
    const unsigned short* in;
    unsigned int i;
    unsigned int s;

    if (!dev || !buf || count == 0) return -1;
    if (dev->sector_size != 512) return -1;
    if (lba + count > dev->sector_count) return -1;
    if (count > 255) return -1;

    ctx = (ata_ctx_t*)dev->ctx;
    if (!ctx) return -1;

    outb(ctx->ctrl, 0x00);
    outb(ctx->io + 6, (unsigned char)((ctx->slave ? ATA_SEL_SLAVE : ATA_SEL_MASTER) | ((lba >> 24) & 0x0Fu)));
    outb(ctx->io + 2, (unsigned char)count);
    outb(ctx->io + 3, (unsigned char)(lba & 0xFFu));
    outb(ctx->io + 4, (unsigned char)((lba >> 8) & 0xFFu));
    outb(ctx->io + 5, (unsigned char)((lba >> 16) & 0xFFu));
    outb(ctx->io + 7, ATA_CMD_WRITE_PIO);

    in = (const unsigned short*)buf;
    for (s = 0; s < count; s++) {
        if (ata_poll_ready(ctx->io, ctx->ctrl) != 0) return -1;
        for (i = 0; i < 256; i++) {
            outw(ctx->io, in[s * 256u + i]);
        }
        io_delay_400ns(ctx->ctrl);
    }

    outb(ctx->io + 7, ATA_CMD_CACHE_FLUSH);
    if (ata_poll_ready(ctx->io, ctx->ctrl) != 0) return -1;
    return 0;
}

static void detect_one(unsigned int index, unsigned short io, unsigned short ctrl, unsigned char slave) {
    unsigned int sectors;
    blockdev_t dev;
    ata_ctx_t* ctx = &g_ata_ctx[index];

    ctx->io = io;
    ctx->ctrl = ctrl;
    ctx->slave = slave;

    if (ata_identify(ctx, &sectors) != 0) {
        console_print("ATA: identify failed on ");
        console_print((io == ATA_PRIMARY_IO) ? "primary " : "secondary ");
        console_print(slave ? "slave\n" : "master\n");
        return;
    }

    memset(&dev, 0, sizeof(dev));
    memcpy(dev.name, "hd", 2);
    dev.name[2] = (char)('0' + (int)block_count());
    dev.name[3] = 0;
    dev.type = BLOCKDEV_TYPE_ATA;
    dev.sector_size = 512;
    dev.sector_count = sectors;
    dev.read = ata_pio_read;
    dev.write = ata_pio_write;
    dev.ctx = ctx;

    if (block_register(&dev) != 0) {
        console_print("BLOCK: registry full, ATA device skipped\n");
        return;
    }

    g_ata_devs[index] = dev;

    console_print("BLOCK: found ");
    console_print(dev.name);
    console_print(" ATA 512B/sector ");
    print_u32(dev.sector_count);
    console_print(" sectors ");
    print_size_gib(dev.sector_count);
    console_print(" GiB\n");
}

void ata_pio_discover(void) {
    size_t before = block_count();

    detect_one(0, ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, 0);
    detect_one(1, ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, 1);
    detect_one(2, ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, 0);
    detect_one(3, ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, 1);

    if (block_count() == before) {
        console_print("BLOCK: no disk found\n");
    }
}
