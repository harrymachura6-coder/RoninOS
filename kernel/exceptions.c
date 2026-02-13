#include "console.h"
#include <stdint.h>

static void print_dec(uint32_t n) {
    char buf[11];
    int i = 0;
    if (n == 0) { console_putc('0'); return; }
    while (n > 0 && i < 10) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) console_putc(buf[i]);
}

void isr_exception_handler(uint32_t int_no, uint32_t err_code) {
    console_print("\nEXCEPTION int=");
    print_dec(int_no);
    console_print(" err=");
    print_dec(err_code);
    console_print("\nSystem halted.\n");
    for (;;) { __asm__ volatile("hlt"); }
}

