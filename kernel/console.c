#include "console.h"

#include "terminal/terminal.h"

void console_clear(uint8_t color) {
    (void)color;
    terminal_clear_scrollback();
}

void console_putc(char c) {
    terminal_putc(c);
}

void console_print(const char* s) {
    terminal_write(s);
}

void console_prompt(void) {
}
