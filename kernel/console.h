#pragma once
#include <stdint.h>

void console_init(uint32_t mb_magic, uint32_t mb_info_addr);
int console_using_framebuffer(void);
int console_get_framebuffer(uint64_t* addr, uint32_t* pitch, uint32_t* w, uint32_t* h, uint32_t* bpp);

void console_clear(uint8_t color);
void console_putc(char c);
void console_print(const char* s);
void console_prompt(void);
