#include "serial.h"

#include "ports.h"

#define COM1_PORT 0x3F8

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
}

static int serial_tx_ready(void) {
    return (inb(COM1_PORT + 5) & 0x20) != 0;
}

void serial_putc(char c) {
    while (!serial_tx_ready()) {
    }
    outb(COM1_PORT, (uint8_t)c);
}

void serial_print(const char* s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s++);
    }
}
