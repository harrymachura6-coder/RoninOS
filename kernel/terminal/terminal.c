#include "terminal.h"

#include "../lib/string.h"
#include "../ports.h"

#define TERM_SCROLLBACK_LINES 512
#define TERM_OUTPUT_ROWS (TERM_HEIGHT - 2)

typedef struct {
    char ch;
    uint8_t attr;
} term_cell_t;

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;

static term_cell_t g_lines[TERM_SCROLLBACK_LINES][TERM_WIDTH];
static uint32_t g_total_lines = 1;
static uint32_t g_current_line = 0;
static size_t g_current_col = 0;
static int g_scroll_offset = 0;
static uint8_t g_current_attr = 0x07;
static terminal_theme_t g_theme;

static char g_prompt[32] = "ronin> ";
static char g_input[128];
static size_t g_input_len = 0;
static size_t g_input_cursor = 0;
static char g_status[80] = "PgUp/PgDn scroll  Up/Down history";

static uint16_t vga_entry(char c, uint8_t attr) {
    return (uint16_t)((uint8_t)c) | ((uint16_t)attr << 8);
}

static uint32_t first_abs_line(void) {
    if (g_total_lines > TERM_SCROLLBACK_LINES) {
        return g_total_lines - TERM_SCROLLBACK_LINES;
    }
    return 0;
}

static void clear_line(uint32_t abs_line) {
    uint32_t idx = abs_line % TERM_SCROLLBACK_LINES;
    size_t i;
    uint8_t attr = theme_attr(g_theme.fg_output, g_theme.bg);

    for (i = 0; i < TERM_WIDTH; i++) {
        g_lines[idx][i].ch = ' ';
        g_lines[idx][i].attr = attr;
    }
}

static void ensure_current_line_initialized(void) {
    if (g_current_col == 0) {
        clear_line(g_current_line);
    }
}

static void linefeed(void) {
    g_current_col = 0;
    g_current_line++;
    g_total_lines = g_current_line + 1;
    clear_line(g_current_line);
    g_scroll_offset = 0;
}

static void set_hw_cursor(size_t row, size_t col) {
    uint16_t pos = (uint16_t)(row * TERM_WIDTH + col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void put_output_char(char c) {
    uint32_t idx;

    if (c == '\n') {
        linefeed();
        return;
    }

    if (c == '\r') {
        g_current_col = 0;
        return;
    }

    if (c == '\b') {
        if (g_current_col > 0) {
            g_current_col--;
            idx = g_current_line % TERM_SCROLLBACK_LINES;
            g_lines[idx][g_current_col].ch = ' ';
            g_lines[idx][g_current_col].attr = g_current_attr;
        }
        return;
    }

    if (c < 32 || c > 126) {
        return;
    }

    ensure_current_line_initialized();
    idx = g_current_line % TERM_SCROLLBACK_LINES;
    g_lines[idx][g_current_col].ch = c;
    g_lines[idx][g_current_col].attr = g_current_attr;
    g_current_col++;

    if (g_current_col >= TERM_WIDTH) {
        linefeed();
    }
}

static int parse_ansi_code(const char* s, size_t* consumed) {
    if (s[0] != '\x1b' || s[1] != '[') return 0;

    if (s[2] == '0' && s[3] == 'm') {
        g_current_attr = theme_attr(g_theme.fg_output, g_theme.bg);
        *consumed = 4;
        return 1;
    }
    if (s[2] == '3' && s[4] == 'm') {
        if (s[3] == '1') {
            g_current_attr = theme_attr(4, g_theme.bg);
            *consumed = 5;
            return 1;
        }
        if (s[3] == '2') {
            g_current_attr = theme_attr(2, g_theme.bg);
            *consumed = 5;
            return 1;
        }
    }
    return 0;
}

void terminal_render(void) {
    uint32_t first = first_abs_line();
    uint32_t bottom = g_current_line;
    uint32_t top;
    size_t row;

    if (g_scroll_offset < 0) g_scroll_offset = 0;
    if (bottom < (uint32_t)g_scroll_offset) {
        g_scroll_offset = (int)bottom;
    }

    top = (bottom >= (TERM_OUTPUT_ROWS - 1 + (size_t)g_scroll_offset))
            ? bottom - (TERM_OUTPUT_ROWS - 1 + (size_t)g_scroll_offset)
            : 0;
    if (top < first) top = first;

    for (row = 0; row < TERM_OUTPUT_ROWS; row++) {
        uint32_t abs_line = top + (uint32_t)row;
        size_t col;

        for (col = 0; col < TERM_WIDTH; col++) {
            term_cell_t cell;
            if (abs_line <= bottom && abs_line >= first) {
                cell = g_lines[abs_line % TERM_SCROLLBACK_LINES][col];
            } else {
                cell.ch = ' ';
                cell.attr = theme_attr(g_theme.fg_output, g_theme.bg);
            }
            VGA[row * TERM_WIDTH + col] = vga_entry(cell.ch, cell.attr);
        }
    }

    {
        size_t row_input = TERM_OUTPUT_ROWS;
        size_t col = 0;
        uint8_t prompt_attr = theme_attr(g_theme.fg_prompt, g_theme.bg);
        uint8_t input_attr = theme_attr(g_theme.fg_input, g_theme.bg);

        while (col < TERM_WIDTH) {
            VGA[row_input * TERM_WIDTH + col] = vga_entry(' ', input_attr);
            col++;
        }

        col = 0;
        while (g_prompt[col] && col < TERM_WIDTH) {
            VGA[row_input * TERM_WIDTH + col] = vga_entry(g_prompt[col], prompt_attr);
            col++;
        }

        {
            size_t i = 0;
            while (i < g_input_len && col < TERM_WIDTH) {
                VGA[row_input * TERM_WIDTH + col] = vga_entry(g_input[i], input_attr);
                col++;
                i++;
            }
        }
    }

    {
        size_t row_status = TERM_HEIGHT - 1;
        size_t col;
        uint8_t status_attr = theme_attr(g_theme.fg_status, 1);

        for (col = 0; col < TERM_WIDTH; col++) {
            char ch = g_status[col] ? g_status[col] : ' ';
            VGA[row_status * TERM_WIDTH + col] = vga_entry(ch, status_attr);
        }
    }

    {
        size_t cursor_col = strlen(g_prompt) + g_input_cursor;
        if (cursor_col >= TERM_WIDTH) cursor_col = TERM_WIDTH - 1;
        set_hw_cursor(TERM_OUTPUT_ROWS, cursor_col);
    }
}

void terminal_init(const terminal_theme_t* theme) {
    size_t i;
    if (theme) {
        g_theme = *theme;
    } else {
        g_theme = THEME_RONIN_DARK;
    }
    g_current_attr = theme_attr(g_theme.fg_output, g_theme.bg);
    g_total_lines = 1;
    g_current_line = 0;
    g_current_col = 0;
    g_scroll_offset = 0;

    for (i = 0; i < TERM_SCROLLBACK_LINES; i++) {
        clear_line((uint32_t)i);
    }

    terminal_render();
}

void terminal_apply_theme(const terminal_theme_t* theme) {
    if (!theme) return;
    g_theme = *theme;
    g_current_attr = theme_attr(g_theme.fg_output, g_theme.bg);
    terminal_render();
}

void terminal_clear_scrollback(void) {
    size_t i;
    g_total_lines = 1;
    g_current_line = 0;
    g_current_col = 0;
    g_scroll_offset = 0;
    g_current_attr = theme_attr(g_theme.fg_output, g_theme.bg);

    for (i = 0; i < TERM_SCROLLBACK_LINES; i++) {
        clear_line((uint32_t)i);
    }

    terminal_render();
}

void terminal_write(const char* s) {
    size_t i = 0;
    while (s && s[i]) {
        size_t consumed = 0;
        if (parse_ansi_code(&s[i], &consumed)) {
            i += consumed;
            continue;
        }
        put_output_char(s[i]);
        i++;
    }
    terminal_render();
}

void terminal_putc(char c) {
    put_output_char(c);
    terminal_render();
}

void terminal_write_error(const char* s) {
    uint8_t old = g_current_attr;
    g_current_attr = theme_attr(g_theme.fg_error, g_theme.bg);
    terminal_write(s);
    g_current_attr = old;
}

void terminal_set_prompt(const char* prompt) {
    size_t i = 0;
    if (!prompt) return;
    while (prompt[i] && i < sizeof(g_prompt) - 1) {
        g_prompt[i] = prompt[i];
        i++;
    }
    g_prompt[i] = 0;
    terminal_render();
}

void terminal_set_input_line(const char* input, size_t len, size_t cursor) {
    size_t i;
    if (!input) {
        g_input[0] = 0;
        g_input_len = 0;
        g_input_cursor = 0;
        terminal_render();
        return;
    }

    if (len >= sizeof(g_input)) {
        len = sizeof(g_input) - 1;
    }

    for (i = 0; i < len; i++) g_input[i] = input[i];
    g_input[len] = 0;
    g_input_len = len;
    g_input_cursor = (cursor > len) ? len : cursor;
    terminal_render();
}

void terminal_scroll_lines(int delta) {
    int max_off;
    uint32_t first = first_abs_line();

    max_off = (int)(g_current_line - first);
    g_scroll_offset += delta;
    if (g_scroll_offset < 0) g_scroll_offset = 0;
    if (g_scroll_offset > max_off) g_scroll_offset = max_off;
    terminal_render();
}

void terminal_set_status(const char* status) {
    size_t i = 0;
    if (!status) return;
    while (status[i] && i < sizeof(g_status) - 1) {
        g_status[i] = status[i];
        i++;
    }
    while (i < sizeof(g_status)) {
        g_status[i++] = 0;
    }
    terminal_render();
}
