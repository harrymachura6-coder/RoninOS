#include "panic.h"

#include "console.h"

static void print_dec(unsigned int n) {
    char buf[12];
    int i = 0;

    if (n == 0) {
        console_putc('0');
        return;
    }

    while (n > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }

    while (i > 0) {
        i--;
        console_putc(buf[i]);
    }
}

void panic(const char* message) {
    __asm__ volatile("cli");
    console_print("\n[PANIC] ");
    console_print(message ? message : "(no message)");
    console_putc('\n');

    for (;;) {
        __asm__ volatile("hlt");
    }
}

void panic_assert_failed(const char* expr, const char* file, int line) {
    __asm__ volatile("cli");
    console_print("\n[ASSERT] ");
    console_print(expr ? expr : "(unknown)");
    console_print(" at ");
    console_print(file ? file : "(unknown)");
    console_putc(':');
    if (line >= 0) {
        print_dec((unsigned int)line);
    } else {
        console_print("?");
    }
    console_putc('\n');

    for (;;) {
        __asm__ volatile("hlt");
    }
}
