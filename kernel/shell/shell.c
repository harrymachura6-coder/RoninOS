#include "shell.h"

#include "../console.h"
#include "../lib/string.h"
#include "../terminal/terminal.h"
#include "commands.h"

#define SHELL_LINE_MAX 128
#define SHELL_HISTORY_MAX 32

static char g_line[SHELL_LINE_MAX];
static size_t g_len = 0;
static size_t g_cursor = 0;

static char g_history[SHELL_HISTORY_MAX][SHELL_LINE_MAX];
static size_t g_hist_count = 0;
static int g_hist_index = -1;
static int g_simple_mode = 0;

static void simple_prompt(void) {
    console_print("ronin> ");
}

static void redraw_input(void) {
    if (g_simple_mode) {
        return;
    }
    terminal_set_input_line(g_line, g_len, g_cursor);
}

static void history_store(void) {
    size_t i;
    if (g_len == 0) return;

    if (g_hist_count > 0 && strcmp(g_history[g_hist_count - 1], g_line) == 0) {
        return;
    }

    if (g_hist_count < SHELL_HISTORY_MAX) {
        i = g_hist_count;
        g_hist_count++;
    } else {
        for (i = 1; i < SHELL_HISTORY_MAX; i++) {
            memcpy(g_history[i - 1], g_history[i], SHELL_LINE_MAX);
        }
        i = SHELL_HISTORY_MAX - 1;
    }
    memcpy(g_history[i], g_line, g_len);
    g_history[i][g_len] = 0;
}

static void history_load(int index) {
    size_t i = 0;
    if (index < 0 || index >= (int)g_hist_count) return;

    while (g_history[index][i] && i < SHELL_LINE_MAX - 1) {
        g_line[i] = g_history[index][i];
        i++;
    }
    g_line[i] = 0;
    g_len = i;
    g_cursor = g_len;
}

void shell_init(void) {
    g_line[0] = 0;
    g_len = 0;
    g_cursor = 0;
    g_simple_mode = console_using_framebuffer();
    if (!g_simple_mode) {
        terminal_set_prompt("ronin> ");
    }
    redraw_input();
    if (g_simple_mode) {
        simple_prompt();
    }
}

void shell_handle_key(key_event_t ev) {
    size_t i;

    if (g_simple_mode) {
        switch (ev.code) {
            case KEY_CHAR:
                if (g_len < SHELL_LINE_MAX - 1 && g_cursor == g_len) {
                    g_line[g_cursor++] = ev.ch;
                    g_len = g_cursor;
                    g_line[g_len] = 0;
                    console_putc(ev.ch);
                }
                return;

            case KEY_BACKSPACE:
                if (g_cursor > 0 && g_cursor == g_len) {
                    g_len--;
                    g_cursor--;
                    g_line[g_len] = 0;
                    console_putc('\b');
                    console_putc(' ');
                    console_putc('\b');
                }
                return;

            case KEY_ENTER:
                console_putc('\n');
                history_store();
                shell_execute_line(g_line);
                g_hist_index = -1;
                g_line[0] = 0;
                g_len = 0;
                g_cursor = 0;
                simple_prompt();
                return;

            default:
                return;
        }
    }

    if (ev.code == KEY_PAGEUP || (ev.code == KEY_UP && (ev.modifiers & KEYMOD_SHIFT))) {
        terminal_scroll_lines(+8);
        return;
    }
    if (ev.code == KEY_PAGEDOWN || (ev.code == KEY_DOWN && (ev.modifiers & KEYMOD_SHIFT))) {
        terminal_scroll_lines(-8);
        return;
    }

    switch (ev.code) {
        case KEY_CHAR:
            if (g_len < SHELL_LINE_MAX - 1) {
                for (i = g_len; i > g_cursor; i--) g_line[i] = g_line[i - 1];
                g_line[g_cursor] = ev.ch;
                g_len++;
                g_cursor++;
                g_line[g_len] = 0;
            }
            break;

        case KEY_BACKSPACE:
            if (g_cursor > 0) {
                for (i = g_cursor - 1; i < g_len - 1; i++) g_line[i] = g_line[i + 1];
                g_len--;
                g_cursor--;
                g_line[g_len] = 0;
            }
            break;

        case KEY_DELETE:
            if (g_cursor < g_len) {
                for (i = g_cursor; i < g_len - 1; i++) g_line[i] = g_line[i + 1];
                g_len--;
                g_line[g_len] = 0;
            }
            break;

        case KEY_LEFT:
            if (ev.modifiers & KEYMOD_CTRL) {
                while (g_cursor > 0 && g_line[g_cursor - 1] == ' ') g_cursor--;
                while (g_cursor > 0 && g_line[g_cursor - 1] != ' ') g_cursor--;
            } else if (g_cursor > 0) {
                g_cursor--;
            }
            break;

        case KEY_RIGHT:
            if (ev.modifiers & KEYMOD_CTRL) {
                while (g_cursor < g_len && g_line[g_cursor] != ' ') g_cursor++;
                while (g_cursor < g_len && g_line[g_cursor] == ' ') g_cursor++;
            } else if (g_cursor < g_len) {
                g_cursor++;
            }
            break;

        case KEY_HOME:
            g_cursor = 0;
            break;

        case KEY_END:
            g_cursor = g_len;
            break;

        case KEY_UP:
            if (g_hist_count > 0) {
                if (g_hist_index < 0) g_hist_index = (int)g_hist_count - 1;
                else if (g_hist_index > 0) g_hist_index--;
                history_load(g_hist_index);
            }
            break;

        case KEY_DOWN:
            if (g_hist_count > 0 && g_hist_index >= 0) {
                g_hist_index++;
                if (g_hist_index >= (int)g_hist_count) {
                    g_hist_index = -1;
                    g_line[0] = 0;
                    g_len = 0;
                    g_cursor = 0;
                } else {
                    history_load(g_hist_index);
                }
            }
            break;

        case KEY_ENTER:
            terminal_putc('\n');
            history_store();
            shell_execute_line(g_line);
            g_hist_index = -1;
            g_line[0] = 0;
            g_len = 0;
            g_cursor = 0;
            break;

        default:
            break;
    }

    redraw_input();
}
