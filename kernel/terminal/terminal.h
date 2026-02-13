#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../ui/theme.h"

#define TERM_WIDTH 80
#define TERM_HEIGHT 25

void terminal_init(const terminal_theme_t* theme);
void terminal_apply_theme(const terminal_theme_t* theme);
void terminal_clear_scrollback(void);
void terminal_write(const char* s);
void terminal_putc(char c);
void terminal_write_error(const char* s);
void terminal_set_prompt(const char* prompt);
void terminal_set_input_line(const char* input, size_t len, size_t cursor);
void terminal_scroll_lines(int delta);
void terminal_set_status(const char* status);
void terminal_render(void);
