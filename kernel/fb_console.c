#include "fb_console.h"

#include <stddef.h>

static volatile uint8_t* g_fb;
static uint32_t g_width;
static uint32_t g_height;
static uint32_t g_pitch;
static uint32_t g_bpp;

static uint32_t g_cols;
static uint32_t g_rows;
static uint32_t g_cursor_x;
static uint32_t g_cursor_y;

#define FB_CHAR_W 8u
#define FB_CHAR_H 16u

static void fb_plot(uint32_t x, uint32_t y, uint32_t rgb) {
    volatile uint8_t* p;
    if (x >= g_width || y >= g_height) return;
    p = g_fb + (size_t)y * g_pitch + (size_t)x * (g_bpp / 8u);
    if (g_bpp == 32) {
        p[0] = (uint8_t)(rgb & 0xFFu);
        p[1] = (uint8_t)((rgb >> 8) & 0xFFu);
        p[2] = (uint8_t)((rgb >> 16) & 0xFFu);
        p[3] = 0;
    } else if (g_bpp == 24) {
        p[0] = (uint8_t)(rgb & 0xFFu);
        p[1] = (uint8_t)((rgb >> 8) & 0xFFu);
        p[2] = (uint8_t)((rgb >> 16) & 0xFFu);
    }
}

static void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb) {
    uint32_t yy;
    for (yy = 0; yy < h; yy++) {
        uint32_t xx;
        for (xx = 0; xx < w; xx++) {
            fb_plot(x + xx, y + yy, rgb);
        }
    }
}

static void draw_cell(char c, uint32_t cx, uint32_t cy) {
    uint32_t x0 = cx * FB_CHAR_W;
    uint32_t y0 = cy * FB_CHAR_H;
    uint32_t fg = 0xE0E0E0u;
    uint32_t bg = 0x101010u;
    uint32_t y;

    fill_rect(x0, y0, FB_CHAR_W, FB_CHAR_H, bg);
    if (c == ' ') return;

    for (y = 0; y < FB_CHAR_H; y++) {
        uint8_t pattern = (uint8_t)(((uint8_t)c << (y & 3u)) ^ (0x5Au + (uint8_t)y));
        uint32_t x;
        for (x = 0; x < FB_CHAR_W; x++) {
            if (pattern & (1u << (x & 7u))) {
                fb_plot(x0 + x, y0 + y, fg);
            }
        }
    }
}

static void scroll_up(void) {
    uint32_t row_bytes;
    uint32_t y;
    volatile uint8_t* dst;
    volatile uint8_t* src;

    if (g_height <= FB_CHAR_H) return;

    row_bytes = g_pitch * FB_CHAR_H;
    dst = g_fb;
    src = g_fb + row_bytes;

    for (y = 0; y < g_height - FB_CHAR_H; y++) {
        uint32_t x;
        for (x = 0; x < g_pitch; x++) {
            dst[(size_t)y * g_pitch + x] = src[(size_t)y * g_pitch + x];
        }
    }

    fill_rect(0, g_height - FB_CHAR_H, g_width, FB_CHAR_H, 0x101010u);
}

int fb_init(uintptr_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp) {
    if (!addr || !width || !height || !pitch) return 0;
    if (!(bpp == 32 || bpp == 24)) return 0;

    g_fb = (volatile uint8_t*)addr;
    g_width = width;
    g_height = height;
    g_pitch = pitch;
    g_bpp = bpp;

    g_cols = width / FB_CHAR_W;
    g_rows = height / FB_CHAR_H;
    if (g_cols == 0 || g_rows == 0) return 0;

    g_cursor_x = 0;
    g_cursor_y = 0;
    fb_clear();
    return 1;
}

void fb_clear(void) {
    if (!g_fb) return;
    fill_rect(0, 0, g_width, g_height, 0x101010u);
    g_cursor_x = 0;
    g_cursor_y = 0;
}

void fb_putc(char c) {
    if (!g_fb) return;

    if (c == '\r') {
        g_cursor_x = 0;
        return;
    }
    if (c == '\n') {
        g_cursor_x = 0;
        g_cursor_y++;
        if (g_cursor_y >= g_rows) {
            scroll_up();
            g_cursor_y = g_rows - 1;
        }
        return;
    }

    draw_cell(c, g_cursor_x, g_cursor_y);
    g_cursor_x++;
    if (g_cursor_x >= g_cols) {
        g_cursor_x = 0;
        g_cursor_y++;
        if (g_cursor_y >= g_rows) {
            scroll_up();
            g_cursor_y = g_rows - 1;
        }
    }
}

void fb_print(const char* s) {
    if (!s) return;
    while (*s) fb_putc(*s++);
}
