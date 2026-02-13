#include "fb_console.h"

#include "lib/string.h"
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

typedef enum {
    FB_FMT_BGRA = 0,
    FB_FMT_RGBA = 1,
} fb_format_t;

#ifdef FB_CONSOLE_FMT_RGBA
static const fb_format_t g_fmt = FB_FMT_RGBA;
#else
static const fb_format_t g_fmt = FB_FMT_BGRA;
#endif

static uint32_t pack_color(uint32_t rgb) {
    uint32_t r = (rgb >> 16) & 0xFFu;
    uint32_t g = (rgb >> 8) & 0xFFu;
    uint32_t b = rgb & 0xFFu;
    if (g_fmt == FB_FMT_BGRA) {
        return b | (g << 8) | (r << 16);
    }
    return r | (g << 8) | (b << 16);
}

static void fb_plot(uint32_t x, uint32_t y, uint32_t rgb) {
    volatile uint8_t* p;
    uint32_t pix;
    if (x >= g_width || y >= g_height) return;
    p = g_fb + (size_t)y * g_pitch + (size_t)x * (g_bpp / 8u);
    pix = pack_color(rgb);
    if (g_bpp == 32) {
        p[0] = (uint8_t)(pix & 0xFFu);
        p[1] = (uint8_t)((pix >> 8) & 0xFFu);
        p[2] = (uint8_t)((pix >> 16) & 0xFFu);
        p[3] = 0;
    } else if (g_bpp == 24) {
        p[0] = (uint8_t)(pix & 0xFFu);
        p[1] = (uint8_t)((pix >> 8) & 0xFFu);
        p[2] = (uint8_t)((pix >> 16) & 0xFFu);
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

static uint8_t glyph5x7_row(char c, uint32_t row) {
    if (row >= 7) return 0;
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    switch (c) {
        case 'A': { static const uint8_t r[7] = {14,17,17,31,17,17,17}; return r[row]; }
        case 'B': { static const uint8_t r[7] = {30,17,17,30,17,17,30}; return r[row]; }
        case 'C': { static const uint8_t r[7] = {14,17,16,16,16,17,14}; return r[row]; }
        case 'D': { static const uint8_t r[7] = {30,17,17,17,17,17,30}; return r[row]; }
        case 'E': { static const uint8_t r[7] = {31,16,16,30,16,16,31}; return r[row]; }
        case 'F': { static const uint8_t r[7] = {31,16,16,30,16,16,16}; return r[row]; }
        case 'G': { static const uint8_t r[7] = {14,17,16,23,17,17,14}; return r[row]; }
        case 'H': { static const uint8_t r[7] = {17,17,17,31,17,17,17}; return r[row]; }
        case 'I': { static const uint8_t r[7] = {31,4,4,4,4,4,31}; return r[row]; }
        case 'J': { static const uint8_t r[7] = {7,2,2,2,2,18,12}; return r[row]; }
        case 'K': { static const uint8_t r[7] = {17,18,20,24,20,18,17}; return r[row]; }
        case 'L': { static const uint8_t r[7] = {16,16,16,16,16,16,31}; return r[row]; }
        case 'M': { static const uint8_t r[7] = {17,27,21,21,17,17,17}; return r[row]; }
        case 'N': { static const uint8_t r[7] = {17,25,21,19,17,17,17}; return r[row]; }
        case 'O': { static const uint8_t r[7] = {14,17,17,17,17,17,14}; return r[row]; }
        case 'P': { static const uint8_t r[7] = {30,17,17,30,16,16,16}; return r[row]; }
        case 'Q': { static const uint8_t r[7] = {14,17,17,17,21,18,13}; return r[row]; }
        case 'R': { static const uint8_t r[7] = {30,17,17,30,20,18,17}; return r[row]; }
        case 'S': { static const uint8_t r[7] = {15,16,16,14,1,1,30}; return r[row]; }
        case 'T': { static const uint8_t r[7] = {31,4,4,4,4,4,4}; return r[row]; }
        case 'U': { static const uint8_t r[7] = {17,17,17,17,17,17,14}; return r[row]; }
        case 'V': { static const uint8_t r[7] = {17,17,17,17,17,10,4}; return r[row]; }
        case 'W': { static const uint8_t r[7] = {17,17,17,21,21,21,10}; return r[row]; }
        case 'X': { static const uint8_t r[7] = {17,17,10,4,10,17,17}; return r[row]; }
        case 'Y': { static const uint8_t r[7] = {17,17,10,4,4,4,4}; return r[row]; }
        case 'Z': { static const uint8_t r[7] = {31,1,2,4,8,16,31}; return r[row]; }
        case '0': { static const uint8_t r[7] = {14,17,19,21,25,17,14}; return r[row]; }
        case '1': { static const uint8_t r[7] = {4,12,4,4,4,4,14}; return r[row]; }
        case '2': { static const uint8_t r[7] = {14,17,1,2,4,8,31}; return r[row]; }
        case '3': { static const uint8_t r[7] = {30,1,1,14,1,1,30}; return r[row]; }
        case '4': { static const uint8_t r[7] = {2,6,10,18,31,2,2}; return r[row]; }
        case '5': { static const uint8_t r[7] = {31,16,16,30,1,1,30}; return r[row]; }
        case '6': { static const uint8_t r[7] = {14,16,16,30,17,17,14}; return r[row]; }
        case '7': { static const uint8_t r[7] = {31,1,2,4,8,8,8}; return r[row]; }
        case '8': { static const uint8_t r[7] = {14,17,17,14,17,17,14}; return r[row]; }
        case '9': { static const uint8_t r[7] = {14,17,17,15,1,1,14}; return r[row]; }
        case '-': { static const uint8_t r[7] = {0,0,0,31,0,0,0}; return r[row]; }
        case '_': { static const uint8_t r[7] = {0,0,0,0,0,0,31}; return r[row]; }
        case '.': { static const uint8_t r[7] = {0,0,0,0,0,6,6}; return r[row]; }
        case ',': { static const uint8_t r[7] = {0,0,0,0,0,6,4}; return r[row]; }
        case ':': { static const uint8_t r[7] = {0,6,6,0,6,6,0}; return r[row]; }
        case ';': { static const uint8_t r[7] = {0,6,6,0,6,4,0}; return r[row]; }
        case '/': { static const uint8_t r[7] = {1,2,2,4,8,8,16}; return r[row]; }
        case '\\': { static const uint8_t r[7] = {16,8,8,4,2,2,1}; return r[row]; }
        case '(': { static const uint8_t r[7] = {2,4,8,8,8,4,2}; return r[row]; }
        case ')': { static const uint8_t r[7] = {8,4,2,2,2,4,8}; return r[row]; }
        case '[': { static const uint8_t r[7] = {14,8,8,8,8,8,14}; return r[row]; }
        case ']': { static const uint8_t r[7] = {14,2,2,2,2,2,14}; return r[row]; }
        case '=': { static const uint8_t r[7] = {0,31,0,31,0,0,0}; return r[row]; }
        case '+': { static const uint8_t r[7] = {0,4,4,31,4,4,0}; return r[row]; }
        case '*': { static const uint8_t r[7] = {0,17,10,4,10,17,0}; return r[row]; }
        case '!': { static const uint8_t r[7] = {4,4,4,4,4,0,4}; return r[row]; }
        case '?': { static const uint8_t r[7] = {14,17,1,2,4,0,4}; return r[row]; }
        case '|': { static const uint8_t r[7] = {4,4,4,4,4,4,4}; return r[row]; }
        case ' ': return 0;
        default: { static const uint8_t r[7] = {14,17,1,2,4,0,4}; return r[row]; }
    }
}

static void draw_cell(char c, uint32_t cx, uint32_t cy) {
    uint32_t x0 = cx * FB_CHAR_W;
    uint32_t y0 = cy * FB_CHAR_H;
    uint32_t fg = 0xE0E0E0u;
    uint32_t bg = 0x000000u;
    uint32_t y;

    if (cx >= g_cols || cy >= g_rows) return;

    for (y = 0; y < FB_CHAR_H; y++) {
        uint32_t py = y0 + y;
        uint32_t font_row = (y >= 1u && y < 15u) ? ((y - 1u) >> 1u) : 7u;
        uint8_t bits = glyph5x7_row(c, font_row);
        uint32_t x;
        if (py >= g_height) continue;
        for (x = 0; x < FB_CHAR_W; x++) {
            uint32_t px = x0 + x;
            uint8_t set = 0;
            if (px >= g_width) continue;
            if (x >= 1u && x <= 5u && bits & (1u << (5u - x))) set = 1;
            fb_plot(px, py, set ? fg : bg);
        }
    }
}

static void scroll_up(void) {
    size_t row_bytes;
    size_t move_bytes;

    if (!g_fb || g_height <= FB_CHAR_H) return;
    row_bytes = (size_t)g_pitch * FB_CHAR_H;
    move_bytes = (size_t)g_pitch * (g_height - FB_CHAR_H);
    memmove((void*)g_fb, (const void*)(g_fb + row_bytes), move_bytes);
    memset((void*)(g_fb + move_bytes), 0, row_bytes);
    g_cursor_y = g_rows - 1u;
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
    size_t fb_bytes;
    if (!g_fb) return;
    fb_bytes = (size_t)g_pitch * g_height;
    if (g_bpp == 32) {
        size_t i;
        volatile uint32_t* fb32 = (volatile uint32_t*)g_fb;
        for (i = 0; i < fb_bytes / 4u; i++) {
            fb32[i] = 0;
        }
    } else {
        memset((void*)g_fb, 0, fb_bytes);
    }
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
