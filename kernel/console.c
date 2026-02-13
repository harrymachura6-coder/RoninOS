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

    serial_init();

    fb = find_mb2_framebuffer(mb_magic, mb_info_addr);
    if (fb && (fb->framebuffer_bpp == 32 || fb->framebuffer_bpp == 24)) {
        if (fb_init((uintptr_t)fb->framebuffer_addr,
                    fb->framebuffer_width,
                    fb->framebuffer_height,
                    fb->framebuffer_pitch,
                    fb->framebuffer_bpp)) {
            g_backend = CONSOLE_BACKEND_FB;
            serial_print("console: framebuffer backend active\n");
            return;
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
