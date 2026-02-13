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

static uint32_t g_font_w;
static uint32_t g_font_h;

#define MAX_FB_COLS 512u
#define MAX_FB_ROWS 256u
static char g_cells[MAX_FB_COLS * MAX_FB_ROWS];
static uint8_t g_track_cells;

typedef enum {
    FB_FMT_BGRA = 0,
    FB_FMT_RGBA = 1,
} fb_format_t;

#ifdef FB_CONSOLE_FMT_RGBA
static const fb_format_t g_fmt = FB_FMT_RGBA;
#else
static const fb_format_t g_fmt = FB_FMT_BGRA;
#endif

#ifndef FB_FONT
#define FB_FONT 1632
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

static uint8_t glyph8x16_row(char c, uint32_t row) {
    static const uint8_t row_map[16] = {255,0,0,1,1,2,2,3,3,4,4,5,5,6,6,255};
    uint8_t src;
    if (row >= 16u) return 0;
    src = row_map[row];
    if (src == 255u) return 0;
    return (uint8_t)(glyph5x7_row(c, src) << 1u);
}

static uint16_t glyph16x32_row(char c, uint32_t row) {
    uint8_t bits8;
    uint16_t bits16 = 0;
    uint32_t x;
    uint32_t src_row;

    if (row >= 32u) return 0;
    src_row = row >> 1u;
    bits8 = glyph8x16_row(c, src_row);
    for (x = 0; x < 8u; x++) {
        if (bits8 & (1u << (7u - x))) {
            bits16 |= (uint16_t)(3u << (14u - x * 2u));
        }
    }
    return bits16;
}

static char cell_get(uint32_t cx, uint32_t cy) {
    if (!g_track_cells || cx >= g_cols || cy >= g_rows) return ' ';
    return g_cells[cy * g_cols + cx];
}

static void cell_set(uint32_t cx, uint32_t cy, char c) {
    if (!g_track_cells || cx >= g_cols || cy >= g_rows) return;
    g_cells[cy * g_cols + cx] = c;
}

static void draw_cell(char c, uint32_t cx, uint32_t cy) {
    uint32_t x0 = cx * g_font_w;
    uint32_t y0 = cy * g_font_h;
    uint32_t fg = 0xE0E0E0u;
    uint32_t bg = 0x000000u;
    uint32_t x;
    uint32_t y;

    if (cx >= g_cols || cy >= g_rows) return;

    for (y = 0; y < g_font_h; y++) {
        for (x = 0; x < g_font_w; x++) {
            fb_plot(x0 + x, y0 + y, bg);
        }
    }

    if (g_font_w == 8u && g_font_h == 16u) {
        for (y = 0; y < 16u; y++) {
            uint8_t bits = glyph8x16_row(c, y);
            for (x = 0; x < 8u; x++) {
                if (bits & (1u << (7u - x))) {
                    fb_plot(x0 + x, y0 + y, fg);
                }
            }
        }
    } else {
        for (y = 0; y < 32u; y++) {
            uint16_t bits = glyph16x32_row(c, y);
            for (x = 0; x < 16u; x++) {
                if (bits & (1u << (15u - x))) {
                    fb_plot(x0 + x, y0 + y, fg);
                }
            }
        }
    }
}

static void draw_cursor(void) {
    uint32_t x0;
    uint32_t y0;
    uint32_t x;
    uint32_t y;

    if (!g_fb || g_cursor_x >= g_cols || g_cursor_y >= g_rows) return;

    x0 = g_cursor_x * g_font_w;
    y0 = g_cursor_y * g_font_h + (g_font_h - (g_font_h / 8u));
    for (y = y0; y < g_cursor_y * g_font_h + g_font_h; y++) {
        for (x = x0; x < x0 + g_font_w; x++) {
            fb_plot(x, y, 0x50A0FFu);
        }
    }
}

static void erase_cursor(void) {
    draw_cell(cell_get(g_cursor_x, g_cursor_y), g_cursor_x, g_cursor_y);
}

static void scroll_up(void) {
    size_t px_row_bytes;
    size_t move_bytes;
    uint32_t clear_y;

    if (!g_fb || g_height <= g_font_h) return;

    px_row_bytes = (size_t)g_pitch * g_font_h;
    move_bytes = (size_t)g_pitch * (g_height - g_font_h);
    memmove((void*)g_fb, (const void*)(g_fb + px_row_bytes), move_bytes);
    memset((void*)(g_fb + move_bytes), 0, px_row_bytes);

    if (g_track_cells && g_rows > 1u) {
        memmove((void*)g_cells, (const void*)(g_cells + g_cols), (size_t)(g_rows - 1u) * g_cols);
        memset((void*)(g_cells + (size_t)(g_rows - 1u) * g_cols), ' ', g_cols);
    }

    clear_y = g_rows - 1u;
    g_cursor_y = clear_y;
}

static void fb_newline(void) {
    g_cursor_x = 0;
    g_cursor_y++;
    if (g_cursor_y >= g_rows) {
        scroll_up();
        g_cursor_y = g_rows - 1u;
    }
}

int fb_init(uintptr_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp) {
    if (!addr || !width || !height || !pitch) return 0;
    if (!(bpp == 32 || bpp == 24)) return 0;

    g_fb = (volatile uint8_t*)addr;
    g_width = width;
    g_height = height;
    g_pitch = pitch;
    g_bpp = bpp;

#if FB_FONT == 816
    g_font_w = 8u;
    g_font_h = 16u;
#else
    g_font_w = 16u;
    g_font_h = 32u;
#endif

    g_cols = width / g_font_w;
    g_rows = height / g_font_h;
    if (g_cols == 0 || g_rows == 0) return 0;

    g_track_cells = (g_cols <= MAX_FB_COLS && g_rows <= MAX_FB_ROWS) ? 1u : 0u;

    g_cursor_x = 0;
    g_cursor_y = 0;
    fb_clear();
    draw_cursor();
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
    if (g_track_cells) {
        memset(g_cells, ' ', (size_t)g_cols * g_rows);
    }
    g_cursor_x = 0;
    g_cursor_y = 0;
    draw_cursor();
}

void fb_putc(char c) {
    if (!g_fb) return;

    erase_cursor();

    if (c == '\r') {
        g_cursor_x = 0;
        draw_cursor();
        return;
    }
    if (c == '\n') {
        fb_newline();
        draw_cursor();
        return;
    }
    if (c == '\b') {
        if (g_cursor_x > 0u) {
            g_cursor_x--;
        } else if (g_cursor_y > 0u) {
            g_cursor_y--;
            g_cursor_x = g_cols - 1u;
        }
        draw_cell(' ', g_cursor_x, g_cursor_y);
        cell_set(g_cursor_x, g_cursor_y, ' ');
        draw_cursor();
        return;
    }

    draw_cell(c, g_cursor_x, g_cursor_y);
    cell_set(g_cursor_x, g_cursor_y, c);
    g_cursor_x++;
    if (g_cursor_x >= g_cols) {
        fb_newline();
    }

    draw_cursor();
}

void fb_print(const char* s) {
    if (!s) return;
    while (*s) fb_putc(*s++);
}

uint32_t fb_console_cols(void) { return g_cols; }
uint32_t fb_console_rows(void) { return g_rows; }
uint32_t fb_console_font_w(void) { return g_font_w; }
uint32_t fb_console_font_h(void) { return g_font_h; }
