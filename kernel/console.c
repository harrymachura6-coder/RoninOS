#include "console.h"

#include "fb_console.h"
#include "serial.h"
#include "terminal/terminal.h"
#include "mem/multiboot2.h"
#include "ui/theme.h"

#include <stddef.h>

typedef enum {
    CONSOLE_BACKEND_TERMINAL = 0,
    CONSOLE_BACKEND_FB = 1,
} console_backend_t;

static console_backend_t g_backend = CONSOLE_BACKEND_TERMINAL;
static uint64_t g_fb_phys_addr;
static uint32_t g_fb_pitch;
static uint32_t g_fb_width;
static uint32_t g_fb_height;
static uint32_t g_fb_bpp;
static int g_fb_valid;

static void fb_print_u32(uint32_t n) {
    char buf[10];
    size_t i = 0;
    if (n == 0) {
        fb_putc('0');
        return;
    }
    while (n && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }
    while (i) {
        fb_putc(buf[--i]);
    }
}

static void fb_print_hex32(uint32_t n) {
    const char* hex = "0123456789ABCDEF";
    int shift;
    fb_print("0x");
    for (shift = 28; shift >= 0; shift -= 4) {
        fb_putc(hex[(n >> (uint32_t)shift) & 0xFu]);
    }
}

static const struct mb2_tag_framebuffer_common* find_mb2_framebuffer(uint32_t mb_magic, uint32_t mb_info_addr) {
    const struct mb2_info_header* hdr;
    const struct mb2_tag* tag;
    uint32_t off;

    if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC || mb_info_addr == 0) {
        return NULL;
    }

    hdr = (const struct mb2_info_header*)(uintptr_t)mb_info_addr;
    off = 8;

    while (off + 8 <= hdr->total_size) {
        tag = (const struct mb2_tag*)((uintptr_t)mb_info_addr + off);
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) {
            break;
        }
        if (tag->type == MULTIBOOT2_TAG_TYPE_FRAMEBUFFER && tag->size >= sizeof(struct mb2_tag_framebuffer_common)) {
            return (const struct mb2_tag_framebuffer_common*)tag;
        }
        off = (off + tag->size + 7u) & ~7u;
    }

    return NULL;
}

void console_init(uint32_t mb_magic, uint32_t mb_info_addr) {
    const struct mb2_tag_framebuffer_common* fb;

    g_fb_phys_addr = 0;
    g_fb_pitch = 0;
    g_fb_width = 0;
    g_fb_height = 0;
    g_fb_bpp = 0;
    g_fb_valid = 0;

    serial_init();

    fb = find_mb2_framebuffer(mb_magic, mb_info_addr);
    if (fb && (fb->framebuffer_bpp == 32 || fb->framebuffer_bpp == 24)) {
        if (fb->framebuffer_addr > 0xFFFFFFFFull) {
            serial_print("console: framebuffer above 4GiB, fallback to VGA terminal\n");
        } else {
            if (fb_init((uintptr_t)fb->framebuffer_addr,
                        fb->framebuffer_width,
                        fb->framebuffer_height,
                        fb->framebuffer_pitch,
                        fb->framebuffer_bpp)) {
                g_backend = CONSOLE_BACKEND_FB;
                g_fb_phys_addr = fb->framebuffer_addr;
                g_fb_pitch = fb->framebuffer_pitch;
                g_fb_width = fb->framebuffer_width;
                g_fb_height = fb->framebuffer_height;
                g_fb_bpp = fb->framebuffer_bpp;
                g_fb_valid = 1;
                serial_print("console: framebuffer backend active\n");
                fb_print("R\xC5\x8DninOS UEFI\n");
                fb_print("Framebuffer: ");
                fb_print_u32(g_fb_width);
                fb_putc('x');
                fb_print_u32(g_fb_height);
                fb_putc('x');
                fb_print_u32(g_fb_bpp);
                fb_print(" pitch=");
                fb_print_u32(g_fb_pitch);
                fb_print(" addr=");
                fb_print_hex32((uint32_t)g_fb_phys_addr);
                fb_putc('\n');
                fb_print("If you see this, framebuffer console works.\n");
                return;
            }
        }
    }

    g_backend = CONSOLE_BACKEND_TERMINAL;
    terminal_init(&THEME_RONIN_DARK);

    if (!fb) {
        serial_print("console: no multiboot2 framebuffer tag, using VGA terminal\n");
    } else if (fb->framebuffer_bpp != 32 && fb->framebuffer_bpp != 24) {
        serial_print("console: unsupported framebuffer bpp, using VGA terminal\n");
    } else {
        serial_print("console: framebuffer init failed, using VGA terminal\n");
    }
}

int console_using_framebuffer(void) {
    return g_backend == CONSOLE_BACKEND_FB;
}

int console_get_framebuffer(uint64_t* addr, uint32_t* pitch, uint32_t* w, uint32_t* h, uint32_t* bpp) {
    if (g_backend != CONSOLE_BACKEND_FB || !g_fb_valid) {
        return 0;
    }

    if (addr) {
        *addr = g_fb_phys_addr;
    }
    if (pitch) {
        *pitch = g_fb_pitch;
    }
    if (w) {
        *w = g_fb_width;
    }
    if (h) {
        *h = g_fb_height;
    }
    if (bpp) {
        *bpp = g_fb_bpp;
    }

    return 1;
}

void console_clear(uint8_t color) {
    (void)color;
    if (g_backend == CONSOLE_BACKEND_FB) {
        fb_clear();
    } else {
        terminal_clear_scrollback();
    }
}

void console_putc(char c) {
    if (g_backend == CONSOLE_BACKEND_FB) {
        fb_putc(c);
    } else {
        terminal_putc(c);
    }
    serial_putc(c);
}

void console_print(const char* s) {
    if (g_backend == CONSOLE_BACKEND_FB) {
        fb_print(s);
    } else {
        terminal_write(s);
    }
    serial_print(s);
}

void console_prompt(void) {
}
