#pragma once
#include <stdint.h>

void console_clear(uint8_t color);
void console_putc(char c);
void console_print(const char* s);
void console_prompt(void);
