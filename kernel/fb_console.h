#pragma once

#include <stdint.h>

int fb_init(uintptr_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp);
void fb_clear(void);
void fb_putc(char c);
void fb_print(const char* s);

uint32_t fb_console_cols(void);
uint32_t fb_console_rows(void);
uint32_t fb_console_font_w(void);
uint32_t fb_console_font_h(void);
